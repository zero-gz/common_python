# -*- coding: utf-8 -*-
import os
import pathspec


class FileProcess(object):
	def __init__(self, root_path, rule_lines=[]):
		self._root_path = root_path
		self._rule_lines = rule_lines
		self._filter_spec = None
		self._process_func = None
		if len(rule_lines) > 0:
			self._filter_spec = pathspec.PathSpec.from_lines('gitwildmatch', self._rule_lines)

	def set_file_process_func(self, func):		
		self._process_func = func

	def file_process(self, path):
		if self._process_func:
			self._process_func(path)

	def visit(self, arg, path, files):
		for filename in files:
			real_path = os.path.join(path, filename)
			if os.path.isfile(real_path):
				if self._filter_spec:
					if self._filter_spec.match_file(real_path):
						self.file_process(real_path)
					else:
						print "filepath=%s not access by rules"%real_path
				else:
					self.file_process(real_path)

	def run(self):				
		os.path.walk(self._root_path, self.visit, None)


def filter_dir_file(root_path, rule_lines, process_func):
	obj = FileProcess(root_path, rule_lines)
	obj.set_file_process_func(process_func)
	obj.run()


if __name__ == "__main__":
	def test_file_process():
		def test(path):
			print "get txt path", path

		filter_dir_file("./", ["**.txt"], test)
	test_file_process()
