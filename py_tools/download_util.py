# -*- encoding: utf-8 -*-
import requests


def download_file(url_path, write_file_name):
    res = requests.get(url_path, stream=True)
    print "get request answer:", res.status_code, res.headers

    if res.status_code != 200:
        print "download %s failed!" % url_path
        return
    with open(write_file_name, "wb") as fp:
        for chunk in res.iter_content(chunk_size=1024):
            if chunk:
                fp.write(chunk)


if __name__ == "__main__":
    download_file("http://www.baidu.com", "test_data.txt")