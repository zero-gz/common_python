# -*- coding:utf-8 -*-
import codecs
import chardet


def test_open_file(filename):
    print "get open file:", filename
    fp = codecs.open(filename, 'r', encoding='utf-8')
    data = fp.read()
    print type(data)
    print data
    fp.close()


def test_str():
    a = '测试一下'
    print type(a)
    print a
    # print a.decode('utf-8').encode('gbk')


def test_unicode():
    # a = u'测试一下'
    # print a
    a = '♠♥♣♦'
    print a, len(a), type(a)
    #print a.decode("utf-8")
    print chardet.detect(a)
    # print a.decode("utf-8").encode("gb2312")


import os


def test_path():
    print "get path:", os.path.join(os.path.dirname(os.path.realpath(__file__)), 'conan.profile')

import unittest
class TestFileEncoding(unittest.TestCase):
    def setUp(self):
        print "---------------  File Encoding Start ---------------"

    def test_case1(self):
        base_dir = os.path.dirname(__file__)
        aim_path = os.path.join(base_dir, "test.txt")
        if not os.path.exists(aim_path):
            aim_path = os.path.join(base_dir, "/test.txt")
        test_open_file(aim_path)

    def test_case2(self):
        test_str()

    def test_case3(self):
        test_unicode()
        test_path()

    def tearDown(self):
        print "---------------  Slots Test End  ----------------"

if __name__ == "__main__":
    unittest.main()
