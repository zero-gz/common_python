# -*- encoding: utf-8 -*-
from lxml import etree
import chardet

"""
lxml对于中文字符的处理比较特殊，如果有中文字段，那么在node结点里面，只有ascii和unicode两种合法形式，node.text = XXX 这个XXXX是有编码要求的，
但是因为text有编码，那么接口的tostring转换成str时，需要指定 encoding为控制台所需要的编码格式
如果写进文件，关键在于write进去的字符串到底是什么编码类型的，读取文件时，最好以 codecs.open方式，指定encoding来进行读取，读取后自动就变成了unicode，
这时候，fromstring是可以直接接受的。

https://www.w3school.com.cn/xpath/xpath_syntax.asp 详解xpath语法，很强大
"""

def test():
    """create xml with etree, just a list for children, and setattr or dict operattion for attribute"""
    root = etree.Element("root", test_tag1="123", test_tag2="abc", test_tag3="10")
    sub_node1 = etree.Element("children1", easy_and_quick="true")
    sub_node1.text = "测试一下".decode("utf-8")
    sub_node2 = etree.Element("children2", just="123", aaa="10")
    root.append(sub_node1)
    root.append(sub_node2)
    xml_str = etree.tostring(root, encoding="utf-8")
    # print type(xml_str), chardet.detect(xml_str), xml_str

    nodes = root.xpath("//root/children1")
    print etree.tostring(nodes[0], encoding="utf-8")

    print "--------------------------------------------"
    return

    """
    new_root = etree.fromstring(xml_str)
    for child in new_root:
        print etree.tostring(child)

    for k, v in new_root[0].items():
        print k, v

    print "-------------------------------------"
    for item in new_root.iter():
        print item.tag, item.text
    """

    # 中文编码的问题
    fp = open("test_xml.txt", "w")
    fp.write(xml_str)
    fp.close()

    import codecs
    fp1 = codecs.open("test_xml.txt", "r", encoding="utf-8")
    new_xml_str = fp1.read()
    print new_xml_str.encode("utf-8")
    print new_xml_str.encode("gbk")
    new_root = etree.fromstring(new_xml_str)
    new_text = new_root[0].text
    print type(new_text), new_text

    fp1.close()



if __name__ == "__main__":
    test()
