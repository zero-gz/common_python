# -*- coding:utf-8 -*-

import subprocess


def get_last_svn_ver(svn_path):
	res = subprocess.Popen(["svn", "info", svn_path], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	svn_info_list = res.stdout.readlines()
	if len(svn_info_list) < 12:
		print "execute [svn info %s] error!"%svn_path
		return
	svn_info = svn_info_list[10:12]
	return "".join(svn_info)


#print get_last_svn_ver(r"F:\workspace\zero-gz.python_common\workspace\simple_tool")