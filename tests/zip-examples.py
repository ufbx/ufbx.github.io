import requests
import os
import shutil
import argparse

example_path = os.path.join("build", "examples")
zip_path = os.path.join("build", "examples-zip")

def zip_examples():
    if os.path.exists(zip_path):
        shutil.rmtree(zip_path)
    os.makedirs(zip_path, exist_ok=True)

    for root, dirs, files in os.walk(example_path):
        rel_path = os.path.relpath(root, example_path)
        if "build_and_run.sh" in files:
            dst_path = os.path.join(zip_path, f"{rel_path}")
            os.makedirs(os.path.dirname(dst_path), exist_ok=True)
            shutil.make_archive(dst_path, "zip", root)

def upload_examples(root_url, api_key):
    total_count = 0
    ok_count = 0
    for root, dirs, files in os.walk(zip_path):
        rel_path = os.path.relpath(root, zip_path)
        for file in files:
            if file.endswith(".zip"):
                path = os.path.join(root, file)
                rel_zip = os.path.join(rel_path, file)
                url = f"{root_url}/{rel_zip}"
                total_count += 1
                with open(path, "rb") as f:
                    r = requests.put(url, f,
                        headers={
                            "Content-Type": "application/zip",
                            "AccessKey": api_key,
                        }
                    )
                    print(f"{url}: {r.status_code}")
                    if 200 <= r.status_code < 300:
                        ok_count += 1

    print(f"\nSuccesfully uploaded {ok_count}/{total_count} files")
    return ok_count == total_count

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--upload-to", help="URL to upload files to")
    parser.add_argument("--api-key", help="CDN API key for uploading")
    args = parser.parse_args()

    zip_examples()

    ok = True
    if args.upload_to:
        url = args.upload_to.rstrip("/")
        if not upload_examples(args.upload_to, args.api_key):
            ok = False
    exit(0 if ok else 1)
