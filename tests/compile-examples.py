import os
import subprocess
import sys
import re
import shutil
import argparse
import stat

g_verbose = True

def build_path(*args):
    return os.path.join("build", *args)

def log(line):
    print(line, flush=True)

def write_lines(path, lines):
    with open(path, "wt", encoding="utf-8", newline="\n") as f:
        f.writelines(line + "\n" for line in lines)

def write_setup_lines(path, lines):
    setup_lines = [
        "#!/usr/bin/env bash",
        "",
        *lines,
    ]
    write_lines(path, setup_lines)

    st = os.stat(path)
    os.chmod(path, st.st_mode | stat.S_IEXEC)

def exec(args, cwd=None, relative_exe=False):
    if g_verbose:
        print_args = args
        if relative_exe:
            print_args[0] = f"./{print_args[0]}"
        if cwd:
            log(f"{cwd}$ " + subprocess.list2cmdline(args))
        else:
            log("$ " + subprocess.list2cmdline(args))
    try:
        exe_args = args
        if relative_exe:
            if cwd:
                exe_abs = os.path.abspath(os.path.join(cwd, args[0]))
            else:
                exe_abs = os.path.abspath(args[0])
            exe_args = [exe_abs] + args[1:]
        return subprocess.check_output(exe_args, stderr=subprocess.STDOUT, cwd=cwd)
    except subprocess.CalledProcessError as exc:
        output = exc.output
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="ignore")
        log(output)
        raise

class Example:
    def __init__(self, name, lang, flags, lines):
        self.name = name
        self.lang = lang
        self.flags = flags
        self.lines = lines
        self.setup_lines = []
        self.path = ""
        self.path_run = ""
        self.crates = []

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

def setup_c(example, info):
    run_path = os.path.join(example.path, "build_and_run.sh")
    write_setup_lines(run_path, example.setup_lines)
    main_path = os.path.join(example.path, "main.c")
    write_lines(main_path, example.lines)

def setup_cpp(example, info):
    run_path = os.path.join(example.path, "build_and_run.sh")
    write_setup_lines(run_path, example.setup_lines)
    main_path = os.path.join(example.path, "main.cpp")
    write_lines(main_path, example.lines)

def setup_rust(example, info):
    run_path = os.path.join(example.path, "build_and_run.sh")
    write_setup_lines(run_path, example.setup_lines)
    os.makedirs(os.path.join(example.path, "src"))
    main_path = os.path.join(example.path, "src", "main.rs")

    crate_versions = {
        "cgmath": "\"0.18.0\"",
        "glam": "\"0.23.0\"",
        "mint": "\"0.5.9\"",
    }

    cargo_lines = []

    crate_name = example.name
    if "/" in crate_name:
        crate_name = crate_name[crate_name.rindex("/") + 1:]

    cargo_lines.extend(l.strip() for l in f"""
        [package]
        name = "{crate_name}"
        version = "0.1.0"
        edition = "2021"
    """.splitlines()[1:-1])

    cargo_lines.append("")

    ufbx_crate = info["ufbx_crate"]
    ufbx_path = info.get("ufbx_rust_path")
    if ufbx_path:
        rel_path = os.path.abspath(ufbx_path)
        ufbx_crate = f"{{ path = \"{rel_path}\" }}"

    cargo_lines.extend(l.strip() for l in f"""
        [dependencies]
        ufbx = {ufbx_crate}
    """.splitlines()[1:-1])

    for crate in example.crates:
        version = crate_versions[crate]
        cargo_lines.append(f"{crate} = {version}")

    cargo_path = os.path.join(example.path, "Cargo.toml")
    write_lines(cargo_path, cargo_lines)
    write_lines(main_path, example.lines)

def diff_lines(a_lines, b_lines):
    num_lines = max(len(a_lines), len(b_lines))
    for i in range(num_lines):
        a = a_lines[i] if i < len(a_lines) else ""
        b = b_lines[i] if i < len(b_lines) else ""
        if a != b:
            print(f"Diff error at line {1+i}:\n{a}\n{b}\n")
            return False
    return True

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-init", action="store_true", help="Skip initialization of compilers")
    parser.add_argument("--ufbx-rust-path", help="Override ufbx Rust crate location")
    parser.add_argument("--example", nargs="+", help="Test single example")
    argv = parser.parse_args()

    file_ext = {
        "c": "c",
        "cpp": "cpp",
        "rust": "rs",
    }

    os.makedirs("build", exist_ok=True)
    os.makedirs(build_path("examples"), exist_ok=True)

    examples = list(search_examples("site"))

    dep_names = {
        "ufbx": [
            ("ufbx.h", "native/viewer/ufbx.h"),
            ("ufbx.c", "native/viewer/ufbx.c"),
        ],
        "example_base": [
            ("example_base.h", "native/example/example_base.h"),
        ],
        "example_math": [
            ("example_math.h", "native/example/example_math.h"),
        ],
    }

    setup_info = {
        "ufbx_crate": "\"0.4.0\"",
        "ufbx_rust_path": None,
    }

    if argv.ufbx_rust_path:
        setup_info["ufbx_rust_path"] = argv.ufbx_rust_path

    setup_fn = {
        "c": setup_c,
        "cpp": setup_cpp,
        "rust": setup_rust,
    }

    dep_paths = [
        "static/models",
    ]

    num_ok = 0
    num_total = 0

    new_examples = []
    for example in examples:
        # compiler = compilers[example.lang]
        key = f"{example.name}-{example.lang}"

        if argv.example:
            if len(argv.example) == 1:
                if example.name != argv.example[0]:
                    continue
            elif len(argv.example) == 2:
                if example.name != argv.example[0]:
                    continue
                if example.lang != argv.example[1]:
                    continue
            else:
                raise RuntimeError("Bad --example, expected 1 or 2 arguments")

        key = key.replace("/", os.sep)
        path = build_path("examples", key)
        path_run = build_path("examples-run", key)
        if os.path.exists(path):
            shutil.rmtree(path)
        os.makedirs(path, exist_ok=True)

        in_meta = True

        example.path = path
        example.path_run = path_run

        frame_name = f"{example.name}.{file_ext[example.lang]}"
        frame_path = os.path.join("tests", "example-frames", frame_name)
        if os.path.exists(frame_path):
            with open(frame_path, "rt", encoding="utf-8") as f:
                new_lines = []
                for line in f.readlines():
                    line = line.rstrip()

                    if in_meta:
                        meta_line = line.strip()
                        if meta_line.startswith("//"):
                            meta_line = meta_line[2:].strip()
                        if meta_line.startswith("$"):
                            tokens = meta_line.split()
                            if tokens[0] == "$dep":
                                for tok in tokens[1:]:
                                    if tok in dep_names:
                                        for dst, src in dep_names[tok]:
                                            dst = os.path.join(path, dst)
                                            shutil.copyfile(src, dst)
                                    else:
                                        for dep_path in dep_paths:
                                            maybe_src = os.path.join(dep_path, tok)
                                            if os.path.exists(maybe_src):
                                                src = maybe_src
                                                break
                                        dst = os.path.join(path, tok)
                                        shutil.copyfile(src, dst)
                            elif tokens[0] == "$crate":
                                example.crates += tokens[1:]
                            else:
                                example.setup_lines.append(meta_line[1:].strip())
                            continue
                        else:
                            in_meta = False

                    if "EXAMPLE_SOURCE" in line:
                        indent = ""
                        m = re.match(r" +", line)
                        if m:
                            indent = m.group(0)
                        new_lines += [indent + l for l in example.lines]
                    else:
                        new_lines.append(line)
                example.lines = new_lines

        setup_fn[example.lang](example, setup_info)
        new_examples.append(example)

    os.makedirs(build_path("examples-run"), exist_ok=True)
    for example in new_examples:
        if os.path.exists(example.path_run):
            shutil.rmtree(example.path_run)
        os.makedirs(example.path_run)
        for root,dirs,files in os.walk(example.path):
            root = os.path.relpath(root, example.path)
            for d in dirs:
                d = os.path.join(root, d)
                os.mkdir(os.path.join(example.path_run, d))

            for f in files:
                f = os.path.join(root, f)
                shutil.copy(
                    os.path.join(example.path, f),
                    os.path.join(example.path_run, f))


        num_total += 1
        try:
            log(f"== {example.name} {example.lang} ==")
            for line in example.setup_lines:
                args = line.split()
                relative_exe = False
                if args[0].startswith("./"):
                    args[0] = args[0][2:]
                    relative_exe = True
                if ">" in args:
                    redirect = args.index(">")
                    args = args[:redirect]
                output = exec(args, cwd=example.path_run, relative_exe=relative_exe)

            output = output.decode("utf-8").splitlines(keepends=False)
            output_path = os.path.join(example.path_run, "output.txt")
            write_lines(output_path, output)

            ref_output_path = os.path.join("tests", "example-outputs", f"{example.name}.txt")
            with open(ref_output_path, "rt", encoding="utf-8") as f:
                ref_output = f.read().splitlines(keepends=False)

            if diff_lines(ref_output, output):
                num_ok += 1
        except subprocess.CalledProcessError:
            pass

    log("")
    if num_ok == num_total:
        log(f"-- OK: {num_ok}/{num_total} succeeded --")
    else:
        log(f"-- FAIL: {num_ok}/{num_total} succeeded --")
        exit(1)

if __name__ == "__main__":
    main()
