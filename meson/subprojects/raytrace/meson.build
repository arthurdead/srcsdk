project('raytrace', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
raytrace_src_dir = join_paths(src_root_dir,'raytrace')

source_base = subproject('source_base')

build_libs = get_option('build_libs')

if build_libs
	raytrace_lib = static_library('raytrace',name_prefix:'',
		cpp_args: [
			'-DLIBNAME=raytrace'
		],
		sources: files(
			join_paths(raytrace_src_dir,'raytrace.cpp'),
			join_paths(raytrace_src_dir,'trace2.cpp'),
			join_paths(raytrace_src_dir,'trace3.cpp'),
		),
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier0'),
			join_paths(src_root_dir,'public','tier1')
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: source_base.get_variable('lib_folder')
	)

	raytrace_dep = declare_dependency(
		link_with: raytrace_lib
	)
else
	raytrace_dep = declare_dependency(
		link_args: [
			'-l:raytrace'+source_base.get_variable('lib_ext')
		]
	)
endif