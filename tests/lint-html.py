from bs4 import BeautifulSoup
import os

htmls = { }
urls = set()

for root, _, files in os.walk("build"):
    root_url = root.replace("\\", "/")
    for name in files:
        if not name.endswith(".html"): continue
        path = os.path.join(root, name)

        url_path = os.path.relpath(path, "build").replace("\\", "/").replace(".html", "")
        if url_path.endswith("index"):
            url_path = url_path[:-5]

        with open(path, "rt") as f:
            soup = BeautifulSoup(f, "html.parser")
            htmls[url_path] = soup

for url, soup in htmls.items():
    urls.add(url)
    for tag in soup.find_all(True):
        id = tag.attrs.get("id")
        if not id: continue
        urls.add(f"{url}#{id}")

num_bad_a_tags = 0
for url, soup in htmls.items():
    for a in soup.find_all("a"):
        href = a.get("href")
        if not href: continue
        if href.startswith("/"):
            if href[1:] not in urls:
                print(f"{url}: <a> href not found: {href}")
                num_bad_a_tags += 1
        elif href.startswith("#"):
            if f"{url}{href}" not in urls:
                print(f"{url}: <a> href not found: {href}")
                num_bad_a_tags += 1
        elif href.startswith("http://") or href.startswith("https://"):
            continue
        else:
            print(f"{url}: <a> with unsupported href: {href}")
            num_bad_a_tags += 1

if num_bad_a_tags > 0:
    raise RuntimeError(f"Error: {num_bad_a_tags} bad <a> tags")

