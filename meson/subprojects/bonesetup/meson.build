project('bonesetup', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
bonesetup_src_dir = join_paths(src_root_dir,'bonesetup')

if build_libs
	bonesetup_lib = static_library('bonesetup',name_prefix:'',
		cpp_args: [
			'-DLIBNAME=bonesetup'
		],
		sources: files(
			join_paths(bonesetup_src_dir,'bone_setup.cpp'),
		),
		include_directories: include_directories(
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: source_base.get_variable('lib_folder')
	)

	bonesetup_dep = declare_dependency(
		link_with: bonesetup_lib
	)
else
	bonesetup_dep = declare_dependency(
		link_args: [
			'-l:bonesetup'+source_base.get_variable('lib_ext')
		]
	)
endif