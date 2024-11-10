project('fgdlib', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

build_libs = get_option('build_libs')

src_root_dir = get_option('src_root_dir')
fgdlib_src_dir = join_paths(src_root_dir,'fgdlib')

source_base = subproject('source_base')

if build_libs
	fgdlib_lib = static_library('fgdlib',name_prefix:'',
		cpp_args: [
			'-DLIBNAME=fgdlib'
		],
		sources: files(
			join_paths(fgdlib_src_dir,'wckeyvalues.cpp'),
			join_paths(fgdlib_src_dir,'inputoutput.cpp'),
			join_paths(fgdlib_src_dir,'gdvar.cpp'),
			join_paths(fgdlib_src_dir,'gdclass.cpp'),
			join_paths(fgdlib_src_dir,'gamedata.cpp'),
		),
		include_directories: include_directories(
			join_paths(src_root_dir,'public','fgdlib'),
			join_paths(src_root_dir,'public','tier0')
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: source_base.get_variable('lib_folder')
	)

	fgdlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','fgdlib')
		),
		link_with: fgdlib_lib
	)
else
	fgdlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','fgdlib')
		),
		link_args: [
			'-l:fgdlib'+source_base.get_variable('lib_ext')
		]
	)
endif