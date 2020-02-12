#-*-coding=utf-8 -*-
# import os,sys
# #列出某个文件夹下的所有 case,这里用的是 python，
# #所在 py 文件运行一次后会生成一个 pyc 的副本
# path="D:\\Python\\Unittest\\search\\test_case"
# caselist=os.listdir(path)
# os.chdir(path)
#
# for a in caselist:
#     s=a.split('.')[1] #选取后缀名为 py 的文件
#     if s=='py':
#     #此处执行 dos 命令并将结果保存到 log.txt
#         os.system('python D:\\Python\\Unittest\\search\\test_case\\%s 1>>D:\\Python\\Unittest\\search\\report\\log.txt 2>&1'%a)

import unittest
import HtmlTestRunner
path = "./test_case"
discover = unittest.defaultTestLoader.discover(path, pattern='*.py')

# 测试
if __name__ == "__main__":
    runner = unittest.TextTestRunner()

    # htmltest runnder has some problem, output has just a litter content
    # fp = open("res.html","wb")
    # runner = HtmlTestRunner.HTMLTestRunner(stream=fp, report_title="simple test", descriptions="test case")
    runner.run(discover)

#测试完成