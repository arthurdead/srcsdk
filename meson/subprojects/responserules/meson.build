project('responserules', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
responserules_runtime_src_dir = join_paths(src_root_dir,'responserules','runtime')

responserules_runtime_lib = static_library('responserules_runtime',
	cpp_args: [
		'-DLIBNAME=responserules_runtime'
	],
	cpp_pch: [
		#join_paths(responserules_runtime_src_dir,'pch','rrbase_pch.h'),
		#join_paths(responserules_runtime_src_dir,'pch','stdafx.cpp')
	],
	sources: files(
		join_paths(responserules_runtime_src_dir,'rrrlib.cpp'),
		join_paths(responserules_runtime_src_dir,'response_types.cpp'),
		join_paths(responserules_runtime_src_dir,'response_types_internal.cpp'),
		join_paths(responserules_runtime_src_dir,'rr_speechconcept.cpp'),
		join_paths(responserules_runtime_src_dir,'criteriaset.cpp'),
		join_paths(responserules_runtime_src_dir,'response_system.cpp'),
		join_paths(responserules_runtime_src_dir,'rr_response.cpp'),
		join_paths(responserules_runtime_src_dir,'rr_convars.cpp'),
	),
	include_directories: include_directories(
		responserules_runtime_src_dir,
		join_paths(public_dir,'tier0'),
		join_paths(public_dir,'tier1')
	),
	dependencies: [
		source_base.get_variable('source_os_static_lib_dep')
	]
)

responserules_runtime_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(public_dir,'responserules')
	),
	link_with: responserules_runtime_lib
)
