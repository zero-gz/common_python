import unreal as ue

def importSimpleAssets():
	texture_tga = r"H:\unreal_project\export_res\ue_export\MyTexture.tga"
	sound_wav = r"H:\unreal_project\export_res\ue_export\Explosion01.WAV"
	texture_task = buildImportTask(texture_tga, "/Game/test_res/Textures")
	sound_task = buildImportTask(sound_wav, "/Game/test_res/Sounds")
	executeImportTasks([texture_task, sound_task])

def buildStaticMeshImportOptions():
	options = ue.FbxImportUI()
	options.set_editor_property("import_mesh", True)
	options.set_editor_property("import_textures", False)
	options.set_editor_property("import_materials", True)
	options.set_editor_property("import_as_skeletal", False)

	options.static_mesh_import_data.set_editor_property("import_translation", ue.Vector(0,0,0))
	options.static_mesh_import_data.set_editor_property("import_rotation", ue.Rotator(0,0,0))
	options.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)

	options.static_mesh_import_data.set_editor_property("combine_meshes", True)
	options.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
	options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
	return options

def buildSkeletalMeshImportOptions():
	options = ue.FbxImportUI()
	options.set_editor_property("import_mesh", True)
	options.set_editor_property("import_textures", False)
	options.set_editor_property("import_materials", True)
	options.set_editor_property("import_as_skeletal", True)

	options.static_mesh_import_data.set_editor_property("import_translation", ue.Vector(0,0,0))
	options.static_mesh_import_data.set_editor_property("import_rotation", ue.Rotator(0,0,0))
	options.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)

	# options.static_mesh_import_data.set_editor_property("import_morph_targets", True)
	#options.static_mesh_import_data.set_editor_property("update_skeleton_reference_pos", False)
	return options

def importMeshAssets():
	static_mesh_fbx = r"H:\unreal_project\export_res\ue_export\chair.FBX"
	skeletal_mesh_fbx = r"H:\unreal_project\export_res\ue_export\SK_Mannequin.FBX"

	static_mesh_task = buildImportTask(static_mesh_fbx, "/Game/test_res/StaticMeshes", buildStaticMeshImportOptions())
	skeletal_mesh_task = buildImportTask(skeletal_mesh_fbx, "/Game/test_res/SkeletalMeshes", buildSkeletalMeshImportOptions())
	executeImportTasks([static_mesh_task, skeletal_mesh_task])

def buildImportTask(filename, dest_path, options = None):
	task = ue.AssetImportTask()
	task.set_editor_property("automated", True)
	task.set_editor_property("destination_name", "")
	task.set_editor_property("destination_path", dest_path)
	task.set_editor_property("filename", filename)
	task.set_editor_property("replace_existing", True)
	task.set_editor_property("save", True)
	task.set_editor_property("options", options)
	return task

def executeImportTasks(tasks):
	ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
	for task in tasks:
		for path in task.get_editor_property("imported_object_paths"):
			print("Imported: %s"%path)

### ------------------------------------------------------------------------------------
def saveAsset(path = "", force_save = True):
	return ue.EditorAssetLibrary.save_asset(asset_to_save = path, only_if_is_dirty = not force_save)

def saveDirectory(path="", force_save=True, recursive=True):
	return ue.EditorAssetLibrary.save_directory(directory_path=path, only_if_is_dirty= not force_save, recursive=recursive)

def getPackageFromPath(path):
	return ue.load_package(path)

def getAllDirtyPackages():
	packages = []
	for content in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages():
		packages.append(content)

	for mapContent in ue.EditorLoadingAndSavingUtils.get_dirty_map_packages():
		packages.append(mapContent)

	return packages

def saveAllDirtyPackages(show_dialog=False):
	if show_dialog:
		return ue.EditorLoadingAndSavingUtils.save_dirty_packages_with_dialog(True, True)
	else:
		return ue.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

def importMyAssets():
	# importSimpleAssets()
	importMeshAssets()
	saveAllDirtyPackages()

#importMyAssets()
