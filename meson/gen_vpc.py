import os, sys, json

meson_info_file = '/code/source/source-sdk-heist/meson/build_windows/everything/meson-info/meson-info.json'
outputdir = '/code/source/source-sdk-heist/meson/vpc'

targets_file = None
buildoptions_file = None
build_dir = None
meson_dir = None
info_dir = None

src_root_dir = None
game_dir = None
src_engine_target = None
src_engine_dir = None

with open(meson_info_file,'r') as content:
	info = json.load(content)

	targets_file = info['introspection']['information']['targets']['file']
	buildoptions_file = info['introspection']['information']['buildoptions']['file']
	build_dir = info['directories']['build']
	info_dir = info['directories']['info']
	meson_dir = info['directories']['source']

with open(os.path.join(info_dir,buildoptions_file),'r') as content:
	opts = json.load(content)

	for opt in opts:
		name = opt['name']
		if name == 'src_root_dir':
			src_root_dir = opt['value']
		elif name == 'game_dir':
			game_dir = opt['value']
		elif name == 'src_engine_target':
			src_engine_target = opt['value']
		elif name == 'src_engine_dir':
			src_engine_dir = opt['value']

		if src_root_dir != None and \
			game_dir != None and \
			src_engine_target != None and \
			src_engine_dir != None:
			break

vpcpath_map = {
	'server': os.path.join(src_root_dir,'game','server'),
	'client': os.path.join(src_root_dir,'game','client'),
	'game_shader_dx9': os.path.join(src_root_dir,'materialsystem','heistshaders'),
	'responserules_runtime': os.path.join(src_root_dir,'responserules','runtime'),
	'choreoobjects': os.path.join(src_root_dir,'choreoobjects'),
	'bonesetup': os.path.join(src_root_dir,'bonesetup'),
	'fgdlib': os.path.join(src_root_dir,'fgdlib'),
	'game_loopback': os.path.join(src_root_dir,'game_loopback'),
	'gamepadui': os.path.join(src_root_dir,'gamepadui_redirect'),
	'GameUI': os.path.join(src_root_dir,'gameui'),
	'hackmgr': os.path.join(src_root_dir,'hackmgr'),
	'matchmaking': os.path.join(src_root_dir,'matchmaking'),
	'mathlib': os.path.join(src_root_dir,'mathlib'),
	'raytrace': os.path.join(src_root_dir,'raytrace'),
	'ServerBrowser': os.path.join(src_root_dir,'serverbrowser_redirect'),
	'shaderlib': os.path.join(src_root_dir,'materialsystem','shaderlib'),
	'tier1': os.path.join(src_root_dir,'tier1'),
	'vgui_controls': os.path.join(src_root_dir,'vgui2','vgui_controls'),
}

game_projects = [
	'server',
	'client',
	'game_shader_dx9',
	'game_loopback',
	'gamepadui',
	'GameUI',
	'hackmgr',
	'matchmaking',
	'ServerBrowser',
]

linux_defines_names = [
	'_GLIBCXX_ASSERTIONS',
	'_FILE_OFFSET_BITS',
	'GNUC',
	'COMPILER_GCC',
	'NO_MALLOC_OVERRIDE',
]

debug_defines_names = [
	'FRAME_POINTER_OMISSION_DISABLED',
	'DEV_BUILD',
	'STAGING_ONLY',
	'DEBUG',
	'_DEBUG',
	'VPROF_ENABLED',
]

release_defines_names = [
	'NDEBUG',
	'_NDEBUG',
]

with open(os.path.join(info_dir,targets_file),'r') as content:
	tgts = json.load(content)

	for tgt in tgts:
		if not tgt['build_by_default']:
			continue

		type = tgt['type']
		if type == 'custom':
			continue

		name = tgt['name']

		if name not in vpcpath_map:
			continue

		with open(os.path.join(outputdir,name+'.vpc'), 'w+') as vpc:

			vpc.write("$Macro SRCDIR \"{}\"\n".format(os.path.relpath(src_root_dir, vpcpath_map[name])))

			is_lib = type == 'static library'
			is_dll = type == 'shared library'

			is_game_project = name in game_projects

			if is_game_project:
				vpc.write('$Macro GAMENAME "heist"\n')

			if is_dll:
				vpc.write("$Macro OUTBINNAME \"{}\"\n".format(name))
			elif is_lib:
				vpc.write("$Macro OUTLIBNAME \"{}\"\n".format(name))

			if is_game_project and is_dll:
				vpc.write('$Macro OUTBINDIR "$SRCDIR\\..\\game\\$GAMENAME\\bin"\n')
			elif is_dll:
				print('wtf')
				os.exit(1)
			elif is_lib:
				installs = tgt['install_filename']
				if len(installs) != 1:
					print('wtf')
					os.exit(1)
				install = installs[0]
				install = os.path.split(install)[0]
				install = install.replace('mingw32/','')
				install = install.replace(src_root_dir,'$SRCDIR')
				vpc.write("$Macro OUTLIBDIR \"{}\"\n".format(install))

			vpc.write('\n')

			if is_lib:
				vpc.write('$Include "$SRCDIR\\vpc_scripts\\source_lib_base.vpc"\n\n')
			elif is_dll:
				vpc.write('$Include "$SRCDIR\\vpc_scripts\\source_dll_base.vpc"\n\n')

			src_files = []
			defines = ['MAKE_VPC']
			debug_defines = debug_defines_names.copy()
			release_defines = release_defines_names.copy()
			incdirs = []

			srcs = tgt['target_sources']
			for src in srcs:
				if 'language' in src and src['language'] == 'cpp':
					cpp_srcs = src['sources']
					for file in cpp_srcs:
						if src_root_dir not in file or \
							'src_funny' in file or \
							build_dir in file or \
							meson_dir in file:
								continue
						file = file.replace(src_root_dir,'$SRCDIR')
						src_files.append("\t\t$File \"{}\"\n".format(file))

					params = src['parameters']
					for param in params:
						start = param[:2]
						if start == '-D':
							define = param[2:]
							if '=' not in define:
								define_name = define
							else:
								define_split = define.split('=',1)
								define_name = define_split[0]
							if define_name in linux_defines_names or \
								define_name == 'FUNNY':
								continue
							if define_name in debug_defines_names:
								if define not in debug_defines:
									debug_defines.append(define)
							elif define_name in release_defines_names:
								if define not in release_defines:
									release_defines.append(define)
							else:
								defines.append(define)
						elif start == '-I':
							inc = param[2:]
							if src_root_dir not in inc or \
								'src_funny' in inc or \
								build_dir in inc or \
								meson_dir in inc:
								continue
							inc = inc.replace(src_root_dir,'$SRCDIR')
							incdirs.append(inc)

			vpc.write('$Configuration "Debug"\n{\n')

			vpc.write('\t$General\n\t{\n')

			if is_game_project:
				vpc.write('\t\t$OutputDirectory ".\\Debug_$GAMENAME"\n')
				vpc.write('\t\t$IntermediateDirectory ".\\Debug_$GAMENAME"\n')

			vpc.write('\t}\n\n')

			vpc.write('\t$Compiler\n\t{\n')

			if len(debug_defines):
				vpc.write('\t\t$PreprocessorDefinitions "$BASE')
				for define in debug_defines:
					vpc.write(";{}".format(define))
				vpc.write('"\n')

			vpc.write('\t}\n}\n\n')

			vpc.write('$Configuration "Release"\n{\n')

			vpc.write('\t$General\n\t{\n')

			if is_game_project:
				vpc.write('\t\t$OutputDirectory ".\\Release_$GAMENAME"\n')
				vpc.write('\t\t$IntermediateDirectory ".\\Release_$GAMENAME"\n')

			vpc.write('\t}\n\n')

			vpc.write('\t$Compiler\n\t{\n')

			if len(release_defines):
				vpc.write('\t\t$PreprocessorDefinitions "$BASE')
				for define in release_defines:
					vpc.write(";{}".format(define))
				vpc.write('"\n')

			vpc.write('\t}\n}\n\n')

			vpc.write('$Configuration\n{\n')

			vpc.write('\t$Compiler\n\t{\n')

			vpc.write('\t\t$AdditionalIncludeDirectories "$BASE')
			for inc in incdirs:
				inc = inc.replace('/', '\\')
				vpc.write(";{}".format(inc))
			vpc.write('"\n')

			vpc.write('\t\t$PreprocessorDefinitions "$BASE')
			for define in defines:
				vpc.write(";{}".format(define))
			vpc.write('"\n')

			vpc.write('\t}\n}\n\n')

			vpc.write("$Project \"{}\"\n{{\n".format(name))

			vpc.write('\t$Folder "Source Files"\n\t{\n')

			for src in src_files:
				src = src.replace('/', '\\')
				vpc.write(src)

			vpc.write('\t}\n')


			vpc.write('}\n')

with open(os.path.join(outputdir,'projects.vgc'), 'w+') as vpc:
	for proj in vpcpath_map:
		loc = vpcpath_map[proj]
		loc = loc.replace(src_root_dir+'/','')
		loc = os.path.join(loc,proj+'.vpc')
		loc = loc.replace('/', '\\')
		vpc.write("$Project \"{}\"\n{{\n\t\"{}\"\n}}\n\n".format(proj,loc))