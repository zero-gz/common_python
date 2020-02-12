# -*- coding:utf-8 -*-


def test1():
    class A(object):
        # __slots__ = ('a', 'b')

        def __init__(self, m):
            self._m = m

        def output(self):
            print self._m

    a = A(3)
    a.b = 2
    a.output()
    a.x = 3


def test2():
    class B():
        pass

    class C(object):
        pass

    b = B()
    c = C()
    print type(b), type(c)

    c.x = 1
    b.x = 1
    print "--------------"



def test3():
    class A(object):
        @staticmethod
        def func1(a, b, c):
            print a, b, c

        @classmethod
        def func2(cls, a=1, b=2):
            print cls, a, b
    A.func1(1, 2, 3)
    A.func2()


import unittest
class TestSlotsAndBaseClass(unittest.TestCase):
    def setUp(self):
        print "---------------  Slots Test Start ---------------"

    def test_case1(self):
        test1()

    def test_case2(self):
        test2()

    def test_case3(self):
        test3()

    def tearDown(self):
        print "---------------  Slots Test End  ----------------"

if __name__ == "__main__":
    unittest.main()
