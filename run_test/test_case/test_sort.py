# -*- coding:utf-8 -*-


class person(object):
    def __init__(self, name, age):
        self._name = name
        self._age = age

    def __str__(self):
        return "get person data:%s %d"%(self._name, self._age)


def compare(person1, person2):
    if person1._age <= person2._age:
        return 1
    else:
        return 0


def test_func():
    person_list = []

    import math
    import random
    for i in xrange(0, 20):
        construct_age = random.randint(10, 30) + i
        construct_name = "%c__%d"%(chr(random.randint(0, 26) + ord('A')), i)
        person_list.append(person(construct_name, construct_age) )

    for p in person_list:
        print p

    print "-----------------------------------"

    def cmp_test(x, y):
        # print type(x), type(y)
        return cmp(x, y)

    #person_list.sort(compare)
    #sorted_list = sorted(person_list, compare)
    #sorted_list = sorted(person_list, key = lambda x:x._age)
    sorted_list = sorted(person_list, cmp = cmp_test, key = lambda x:x._age)

    for p in sorted_list:
        print p


import unittest
class TestListSort(unittest.TestCase):
    def setUp(self):
        print "--------  List Sort Test Start -----------"

    def test_list_sort(self):
        test_func()

    def tearDown(self):
        print "--------  List Sort Test End -------------"


if __name__ == "__main__":
    unittest.main()
