import os
import subprocess
import sys
import re
import shutil
import argparse
from collections import namedtuple

g_verbose = True

def build_path(*args):
    return os.path.join("build", *args)

def log(line):
    print(line, flush=True)

def write_lines(path, lines):
    with open(path, "wt", encoding="utf-8") as f:
        f.writelines(line + "\n" for line in lines)

def exec(args, cwd=None, relative_exe=False):
    if g_verbose:
        if cwd:
            log(f"{cwd}$ " + subprocess.list2cmdline(args))
        else:
            log("$ " + subprocess.list2cmdline(args))
    try:
        exe_args = args
        if relative_exe:
            exe_abs = os.path.abspath(args[0])
            exe_args = [exe_abs] + args[1:]
        return subprocess.check_output(exe_args, stderr=subprocess.STDOUT, cwd=cwd)
    except subprocess.CalledProcessError as exc:
        output = exc.output
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="ignore")
        log(output)
        raise

class Compiler:
    def __init__(self, lang):
        self.lang = lang
        self.compiler_path = build_path(f"compiler-{lang}")
        if sys.platform == "win32":
            self.exe_path = "example.exe"
        else:
            self.exe_path = "example"

    def prepare(self):
        pass

    def run(self, path):
        exe_path = os.path.join(path, self.exe_path)
        return exec([exe_path], cwd=build_path("examples"), relative_exe=True)

class CompilerC(Compiler):
    def __init__(self, lang):
        super().__init__(lang)
        if sys.platform == "win32":
            self.ufbx_obj = os.path.join(self.compiler_path, "ufbx.obj")
        else:
            self.ufbx_obj = os.path.join(self.compiler_path, "ufbx.o")

    def setup(self, path, source_lines):
        main_path = os.path.join(path, "main.c")
        write_lines(main_path, source_lines)

class CompilerGCC(CompilerC):
    def __init__(self, lang, args, default_exe=None):
        super().__init__(lang)
        if not default_exe:
            default_exe = "g++" if lang == "cpp" else "gcc"
        self.exe = args.get("exe", default_exe)
        if isinstance(self.exe, str):
            self.exe = [self.exe]

    def check(self):
        args = self.exe + ["--version"]
        exec(args)

    def prepare(self):
        args = self.exe + ["-c", "native/viewer/ufbx.c", "-o", self.ufbx_obj]
        exec(args)

    def compile(self, path):
        main_path = os.path.join(path, "main.c")
        exe_path = os.path.join(path, self.exe_path)
        lm = []
        if sys.platform != "win32":
            lm = ["-lm"]
        args = self.exe + ["-I", "native/viewer"] + lm + [self.ufbx_obj, main_path, "-o", exe_path]
        exec(args)

class CompilerClang(CompilerGCC):
    def __init__(self, lang, args, default_exe=None):
        if not default_exe:
            default_exe = "clang++" if lang == "cpp" else "clang"
        super().__init__(lang, args, default_exe)

class CompilerCargo(Compiler):
    def __init__(self, lang, args, default_exe="cargo"):
        super().__init__(lang)
        self.exe = args.get("exe", default_exe)
        if isinstance(self.exe, str):
            self.exe = [self.exe]

        if sys.platform == "win32":
            self.exe_path = os.path.join("target", "debug", "example.exe")
        else:
            self.exe_path = os.path.join("target", "debug", "example")

    def check(self):
        args = self.exe + ["--version"]
        exec(args)

    def prepare(self):
        path = self.compiler_path

        exec(self.exe + ["init"], cwd=path)

        def patch_lines(lines):
            for line in lines:
                line = line.strip()
                if "[dependencies]" in line:
                    yield "[dependencies]"
                    yield "ufbx = \"0.3.0\""
                elif "name = " in line:
                    yield "name = \"example\""
                else:
                    yield line

        toml_path = os.path.join(path, "Cargo.toml")
        with open(toml_path, "rt", encoding="utf-8") as f:
            toml_lines = list(patch_lines(f))
        write_lines(toml_path, toml_lines)

        exec(self.exe + ["build"], cwd=path)

    def setup(self, path, source_lines):
        shutil.copytree(self.compiler_path, path, dirs_exist_ok=True)

        main_path = os.path.join(path, "src", "main.rs")
        write_lines(main_path, source_lines)

    def compile(self, path):
        exec(self.exe + ["build"], cwd=path)

compiler_factories = {
    "gcc": CompilerGCC,
    "clang": CompilerClang,
    "cargo": CompilerCargo,
}

Example = namedtuple("Example", "name lang flags lines")

def search_examples(path):
    seen_examples = set()
    for root, _, files in os.walk(path):
        for file in files:
            if not file.endswith(".md"): continue
            path = os.path.join(root, file)

            with open(path, "rt", encoding="utf-8") as f:
                code_lang = None
                code_name = None
                code_flags = []
                code_lines = []
                for line in f:
                    line = line.rstrip()
                    m = re.match(r"\s*```(\w+)\s*", line)
                    if m:
                        code_lang = m.group(1)
                        code_name = None
                        code_flags = []
                        code_lines = []
                        continue
                    m = re.match(r"\s*(?://|#)\s*ufbx-doc-example\s*:\s*(.*)", line)
                    if m:
                        code_flags = m.group(1).split()
                        code_name = code_flags[0]
                        code_flags = code_flags[1:]
                        continue
                    m = re.match(r"\s*```\s*", line)
                    if m:
                        if code_lang and code_name:
                            key = f"{code_name}-{code_lang}"
                            if key in seen_examples:
                                raise RuntimeError(f"Duplicate example {key}")
                            seen_examples.add(key)
                            yield Example(code_name, code_lang, code_flags, code_lines)
                        code_lang = None
                        code_name = None
                        code_lines = []
                        continue
                    code_lines.append(line)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-init", action="store_true", help="Skip initialization of compilers")
    argv = parser.parse_args()

    compilers = { }

    config_default = {
        "compilers": {
            "c": {
                "type": "clang",
            },
            "cpp": {
                "type": "clang",
            },
            "rust": {
                "type": "cargo",
            },
        }
    }

    file_ext = {
        "c": "c",
        "cpp": "cpp",
        "rust": "rs",
    }

    os.makedirs("build", exist_ok=True)

    config = config_default
    for lang, compiler_config in config["compilers"].items():
        type_ = compiler_config["type"]
        compilers[lang] = compiler_factories[type_](lang, compiler_config)

        compiler_path = build_path(f"compiler-{lang}")
        if os.path.exists(compiler_path) and not argv.skip_init:
            shutil.rmtree(compiler_path)
        os.makedirs(compiler_path, exist_ok=True)

    if not argv.skip_init:

        log(f"== Checking compilers ==")
        for compiler in compilers.values():
            compiler.check()

        for compiler in compilers.values():
            log("")
            log(f"== Preparing compiler: {compiler.lang} ==")
            compiler.prepare()

    os.makedirs(build_path("examples"), exist_ok=True)
    shutil.copyfile("static/models/blender_default_cube.fbx", build_path("examples", "my_scene.fbx"))

    examples = list(search_examples("site"))

    for example in examples:
        log("")
        log(f"== Compiling: {example.name} {example.lang} ==")

        compiler = compilers[example.lang]
        key = f"{example.name}-{example.lang}"
        path = build_path("examples", key)
        if os.path.exists(path):
            shutil.rmtree(path)
        os.makedirs(path, exist_ok=True)

        frame_name = f"{example.name}.{file_ext[example.lang]}"
        frame_path = os.path.join("tests", "example-frames", frame_name)
        if os.path.exists(frame_path):
            with open(frame_path, "rt", encoding="utf-8") as f:
                new_lines = []
                for line in f.readlines():
                    line = line.rstrip()
                    if "EXAMPLE_SOURCE" in line:
                        new_lines += example.lines
                    else:
                        new_lines.append(line)
                example = example._replace(lines=new_lines)

        compiler.setup(path, example.lines)
        compiler.compile(path)

        log("")
        log(f"== Executing: {example.name} {example.lang} ==")

        output = compiler.run(path)
        if isinstance(output, bytes):
            output = output.decode("utf-8")
        output = output.splitlines(keepends=False)

        output_path = os.path.join(path, "output.txt")
        write_lines(output_path, output)

        ref_output_path = os.path.join("tests", "example-outputs", f"{example.name}.txt")
        with open(ref_output_path, "rt", encoding="utf-8") as f:
            ref_output = f.read().splitlines(keepends=False)
        
        if output != ref_output:
            log("")
            log("-- FAILED --")
            log("")
            log(f"Expected output ({ref_output_path}):")
            log("\n".join(ref_output))
            log("")
            log(f"Got output ({output_path}):")
            log("\n".join(output))
            raise RuntimeError("Bad output")

if __name__ == "__main__":
    main()

log("")
log("-- SUCCESS --")
