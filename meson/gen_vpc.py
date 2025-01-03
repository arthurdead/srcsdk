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

with open(os.path.join(info_dir,targets_file),'r') as content:
	tgts = json.load(content)

	for tgt in tgts:
		if not tgt['build_by_default']:
			continue

		type = tgt['type']
		if type == 'custom':
			continue

		name = tgt['name']

		with open(os.path.join(outputdir,name+'.vpc'), 'w+') as vpc:

			if type == 'static library':
				vpc.write('$Include "$SRCDIR\\vpc_scripts\\source_lib_base.vpc"\n\n')
			elif type == 'shared library':
				vpc.write('$Include "$SRCDIR\\vpc_scripts\\source_dll_base.vpc"\n\n')

			src_files = []
			defines = []
			incdirs = []

			srcs = tgt['target_sources']
			for src in srcs:
				if 'language' in src and src['language'] == 'cpp':
					cpp_srcs = src['sources']
					for file in cpp_srcs:
						if build_dir in file or meson_dir in file:
								continue
						file = file.replace(src_root_dir,'$SRCDIR')
						src_files.append("\t\t$File \"{}\"\n".format(file))

					params = src['parameters']
					for param in params:
						start = param[:2]
						if start == '-D':
							defines.append(param[2:])
						elif start == '-I':
							inc = param[2:]
							if build_dir in inc or meson_dir in inc:
								continue
							inc = inc.replace(src_root_dir,'$SRCDIR')
							incdirs.append(inc)

			vpc.write('$Configuration\n{\n')

			vpc.write('\t$Compiler\n\t{\n')

			vpc.write('\t\t$AdditionalIncludeDirectories "$BASE;')
			for inc in incdirs:
				inc = inc.replace('/', '\\')
				vpc.write("{};".format(inc))
			vpc.write('"\n')

			vpc.write('\t\t$PreprocessorDefinitions "$BASE;')
			for define in defines:
				vpc.write("{};".format(define))
			vpc.write('"\n')

			vpc.write('}\n\n')

			vpc.write("$Project \"{}\"\n{{\n".format(name))

			vpc.write('\t$Folder "Source Files"\n\t{\n')

			for src in src_files:
				src = src.replace('/', '\\')
				vpc.write(src)

			vpc.write('\t}\n')


			vpc.write('}\n')
