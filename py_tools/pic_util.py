# -*- encoding: utf-8 -*-
import hashlib
import struct
from PIL import Image

"""
	get pic file normal data, such as texture mode, size, md5
	and  dds special func, get mipmapcount and so on   
"""


def get_texture_mode(file_path):
	"""return the rgba or rgb mode"""
	try:
		if file_path.endswith(".dds"):
			with open(file_path) as file:
				data = file.read(100)
				bit_count = struct.unpack('I', data[88:92])
				return (bit_count[0] == 24) and "RGB" or "RGBA"

		img = Image.open(file_path)
		mode = img.mode
		img.close()

		return mode
	except:
		return None


def get_texture_size(file_path):
	"""return the texture size"""
	try:
		if file_path.endswith(".dds"):
			with open(file_path) as fp:
				data = fp.read(20)
				h, w = struct.unpack('II', data[12:])
				return w, h

		if file_path.endswith(".tga"):
			with open(file_path) as fp:
				data = fp.read(16)
				w, h = struct.unpack("hh", data[12:])
				return w, h
	except:
		pass

	try:
		img = Image.open(file_path)
		w, h = img.size
		img.close()

		return w, h
	except:
		return None, None


def is_texture_square_pot(file_path):
	"""return the texture size is pow 2 size"""
	w,h = get_texture_size(file_path)
	if w == h and w & (w-1) == 0:
		return True, w

	max_size = max(w, h)

	count = 1
	while True:
		count = count*2
		if count >= max_size:
			break

	return False, count


def get_file_md5(file_path):
	"""return the all content md5"""
	hash_md5 = hashlib.md5()
	with open(file_path, "rb") as f:
		for chunk in iter(lambda: f.read(4096), b""):
			hash_md5.update(chunk)
	return hash_md5.hexdigest()


#
#  DDS process part
#

def get_dds_mipmap_count(file_path):
	"""return the dds mipmap count,can only process dds file"""
	try:
		if file_path.endswith(".dds"):
			with open(file_path) as file:
				data = file.read(100)
				bit_count = struct.unpack('I', data[28:32])
				return bit_count[0]
	except:
		return None


def is_dds_support_file(file_path):
	"""this func can check DXT type, and the bits"""
	if not file_path.endswith(".dds"):
		return False
	with open(file_path) as fp:
		data = fp.read(100)
		tmp_data1 = struct.unpack('4s', data[84:88])
		name = tmp_data1[0]
		if name in ['DXT1', 'DXT3', 'DXT5']:
			return False

		tmp_data2 = struct.unpack('I', data[88:92])
		bit_count = tmp_data2[0]
		if bit_count != 24 and bit_count != 32:
			return False

		return True