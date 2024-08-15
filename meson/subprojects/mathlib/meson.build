project('mathlib', 'cpp')

src_root_dir = get_option('src_root_dir')
mathlib_src_dir = join_paths(src_root_dir,'mathlib')

source_base = subproject('source_base')

mathlib_lib = static_library('mathlib',
	cpp_args: [
		'-DMATHLIB_LIB'
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
		join_paths(mathlib_src_dir,'gcc_fix.cpp')
	),
	include_directories: include_directories(
		join_paths(src_root_dir,'public','mathlib'),
		join_paths(src_root_dir,'public','tier0')
	),
	dependencies: [
		source_base.get_variable('source_linux_static_lib_dep')
	]
)

mathlib_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','mathlib')
	),
	link_with: mathlib_lib
)
