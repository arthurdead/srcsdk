project('mathlib', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

build_libs = get_option('build_libs')

src_root_dir = get_option('src_root_dir')
mathlib_src_dir = join_paths(src_root_dir,'mathlib')

source_base = subproject('source_base')

if build_libs
	mathlib_lib = static_library('mathlib',name_prefix:'',
		cpp_args: [
			'-DMATHLIB_LIB',
			'-DLIBNAME=mathlib'
		],
		sources: files(
			join_paths(mathlib_src_dir,'sse.cpp'),
			join_paths(mathlib_src_dir,'vmatrix.cpp'),
			join_paths(mathlib_src_dir,'ssenoise.cpp'),
			join_paths(mathlib_src_dir,'anorms.cpp'),
			join_paths(mathlib_src_dir,'powsse.cpp'),
			join_paths(mathlib_src_dir,'imagequant.cpp'),
			join_paths(mathlib_src_dir,'sseconst.cpp'),
			join_paths(mathlib_src_dir,'bumpvects.cpp'),
			join_paths(mathlib_src_dir,'spherical.cpp'),
			join_paths(mathlib_src_dir,'sparse_convolution_noise.cpp'),
			join_paths(mathlib_src_dir,'IceKey.cpp'),
			join_paths(mathlib_src_dir,'quantize.cpp'),
			join_paths(mathlib_src_dir,'lightdesc.cpp'),
			join_paths(mathlib_src_dir,'3dnow.cpp'),
			#join_paths(mathlib_src_dir,'vector.cpp'),
			join_paths(mathlib_src_dir,'polyhedron.cpp'),
			join_paths(mathlib_src_dir,'almostequal.cpp'),
			join_paths(mathlib_src_dir,'simdvectormatrix.cpp'),
			join_paths(mathlib_src_dir,'halton.cpp'),
			join_paths(mathlib_src_dir,'color_conversion.cpp'),
			join_paths(mathlib_src_dir,'randsse.cpp'),
			join_paths(mathlib_src_dir,'mathlib_base.cpp'),
			join_paths(mathlib_src_dir,'linker_fixes.cpp')
		),
		include_directories: include_directories(
			join_paths(src_root_dir,'public','mathlib'),
			join_paths(src_root_dir,'public','tier0')
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: join_paths(src_root_dir,'lib','public','linux32')
	)

	mathlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','mathlib')
		),
		link_with: mathlib_lib
	)
else
	mathlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','mathlib')
		),
		link_args: [
			'-l:mathlib.a'
		]
	)
endif