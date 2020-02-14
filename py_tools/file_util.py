#-*- coding:utf-8 -*-
import shutil
import os


def safe_remove_files(src):
	"""can remove files or dirs"""
	if os.path.isdir(src):
		shutil.rmtree(src)
	elif os.path.isfile(src):
		os.remove(src)
	return True


def safe_copy_files(src, dst, with_delete=False):
	"""can copy files or dirs"""
	if os.path.isdir(src):
		if os.path.exists(dst):
			if with_delete:
				safe_remove_files(dst)
			else:
				file_list = os.listdir(src)
				for file_dir_name in file_list:
					new_src = os.path.join(src, file_dir_name)
					new_dst = os.path.join(dst, file_dir_name)
					safe_copy_files(new_src, new_dst, with_delete)
				return True
		shutil.copytree(src, dst)
	elif os.path.isfile(src):
		shutil.copy(src, dst)
	else:
		print "src %s is not file and dir"%src
		return False

	return True
