project('choreoobjects', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
choreoobjects_src_dir = join_paths(src_root_dir,'choreoobjects')

if build_libs
	choreoobjects_lib = static_library('choreoobjects',name_prefix:'',
		cpp_args: [
			'-DLIBNAME=choreoobjects'
		],
		sources: files(
			join_paths(choreoobjects_src_dir,'choreoactor.cpp'),
			join_paths(choreoobjects_src_dir,'choreochannel.cpp'),
			join_paths(choreoobjects_src_dir,'choreoevent.cpp'),
			join_paths(choreoobjects_src_dir,'choreoscene.cpp'),
			join_paths(choreoobjects_src_dir,'sceneimage.cpp'),
			join_paths(choreoobjects_src_dir,'scenetokenprocessor.cpp'),
		),
		include_directories: include_directories(
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(public_dir,'choreoobjects'),
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: source_base.get_variable('lib_folder')
	)

	choreoobjects_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(public_dir,'choreoobjects'),
		),
		link_with: choreoobjects_lib
	)
else
	choreoobjects_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(public_dir,'choreoobjects'),
		),
		link_args: [
			'-l:choreoobjects'+source_base.get_variable('lib_ext')
		]
	)
endif