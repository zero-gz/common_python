import os, sys
import shutil

DEST_ROOT = r'H:\test_engine'
DestEnginePluginsDir = os.path.abspath(os.path.join(DEST_ROOT, 'Engine/Plugins'))
DestEngineBinariesDir = os.path.abspath(os.path.join(DEST_ROOT, 'Engine/Binaries'))

CurrentPath = os.path.dirname(os.path.abspath(__file__))
EngineDir = os.path.abspath(os.path.join(CurrentPath, 'Engine'))
EnginePluginsDir = os.path.abspath(os.path.join(EngineDir, 'Plugins'))
EngineBinariesDir = os.path.abspath(os.path.join(EngineDir, 'Binaries'))

PdbsToCopy = []#["UE4Editor-MyProject.pdb", ]

def copy_to_dest(rel_dir, dest_dir):
	for root, dirs, files in os.walk(rel_dir):
		for f in files:
			if f.endswith(".pdb") and f not in PdbsToCopy:
				continue

			filename = os.path.join(root, f)
			dest_file = os.path.join(dest_dir, filename)
			print(filename)
			print("---->" + dest_file)
			dest_file_dir = os.path.dirname(dest_file)
			if not os.path.exists(dest_file_dir):
				os.makedirs(dest_file_dir)
			shutil.copy(filename, dest_file_dir)

if __name__ == "__main__":
	try:
		os.chdir(EnginePluginsDir)
		for root, dirs, files in os.walk("."):
			if root.endswith(r"\Resources") or root.endswith(r"\Paper2D"):
				#print(root)
				copy_to_dest(root, DestEnginePluginsDir)
			
			if root.endswith(r"\Binaries"):
				for dir in dirs:
					dir_to_copy = os.path.join(root, dir)
					print(dir_to_copy)
					copy_to_dest(dir_to_copy, DestEnginePluginsDir)

		os.chdir(EngineBinariesDir)
		for root, dirs, files in os.walk("."):
			if root == ".":
				for d in dirs:
					copy_to_dest(d, DestEngineBinariesDir)
			
	except Exception as ex:
		print(ex)
		print("------------- Copy Binaries failed -------------")
		sys.exit(-1)
