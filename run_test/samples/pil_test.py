from PIL import Image
from PIL import ImageFilter
import os
from colorama import Fore
from colorama import init as colorma_init
import numpy as np

colorma_init(autoreset=True)

#print type(Fore.RED), Fore.RED+"123"

# ROOT_PATH = "res/texture/"
# for filepath in os.listdir(ROOT_PATH):
# 	#print "get filepath", ROOT_PATH + filepath
# 	real_path = ROOT_PATH + filepath
# 	if os.path.isfile(real_path):
# 		try:
# 			with Image.open(real_path) as im:
# 				print Fore.GREEN + "%s:\n" % (ROOT_PATH + filepath)
# 				print im.format, im.size, im.mode
# 		except Exception as e:
# 			print Fore.GREEN+"%s:\n"%(ROOT_PATH+filepath) + Fore.RED+"get error msg:%s"%(repr(e.args) )

one_path = "res/texture/test.jpg"

im = Image.open(one_path)
# new_im = im.crop((100, 100, 200, 200))
# new_im.save("res/texture/output.png")
# new_im.close()

# data_list = im.split()
# print len(data_list)
#
# data_list[1].save("green.png")

#im.show()
# count = 0
# def point_func(R):
# 	print "111111111", type(R)
#  	global count
#  	count += 1
#  	print "get data", R
#
# data_list[1].point(point_func)

import re
reg_compile = re.compile("[A-Z]+")
# for str_tmp in ["abc", "ABCD"]:
#  	ret = reg_compile.match(str_tmp)
# 	if ret:
# 		print ret.group()

# count = 0
# for data in dir(ImageFilter):
# 	ret = reg_compile.match(data)
# 	if ret:
# 		count += 1
# 		new_file_name = "res/texture/tmp/%s_%d.png"%(data, count)
# 		try:
# 			out = im.filter(getattr(ImageFilter, data))
# 		except Exception as e:
# 			print Fore.RED + "get error:%s"%e.args
# 		out.save(new_file_name)
# 		#print getattr(ImageFilter, data)


# for filter_type in ImageFilter:
# 	print filter_type

# width, height = im.size
# print width,height
#
# count = 0
# for i in xrange(width):
# 	for j in xrange(height):
# 		pix = im.getpixel((i,j) )
# 		count += 1
# 		#print pix
# print "get count:", count, 1024*768

im.close()

# my_im = Image.new("RGBA", (100, 100), 0xff000000)

my_data_list = []
width, height = 100, 100
for j in xrange(height):
	my_data_list.append([])
	for i in xrange(width):
		my_data_list[j].append([255, 0, 0])

np_data = np.array(my_data_list)
print "get data", np_data[0][0], len(np_data[0])
my_im = Image.fromarray(np.uint8(np_data))

my_im.save("my_output.png")
my_im.close()

# print "total count", count