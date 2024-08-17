project('video_services', 'cpp')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
video_services_src_dir = join_paths(src_root_dir,'video_services','video_services')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
tier2 = subproject('tier2')
tier3 = subproject('tier3')
vstdlib = subproject('vstdlib')

cmake = import('cmake')

webm_opts = cmake.subproject_options()
webm_opts.add_cmake_defines({
	'CMAKE_POSITION_INDEPENDENT_CODE': true
})
webm = cmake.subproject('webm', options: webm_opts)

video_services_lib = shared_library('video_services',name_prefix:'',
	cpp_args: [
		'-DMPI',
		'-DPROTECTED_THINGS_DISABLE'
	],
	sources: files(
		join_paths(video_services_src_dir,'video_services.cpp'),
		join_paths(video_services_src_dir,'video_material.cpp'),
		join_paths(video_services_src_dir,'OpusVorbisDecoder.cpp'),
		join_paths(video_services_src_dir,'VPXDecoder.cpp'),
		join_paths(video_services_src_dir,'WebMDemuxer.cpp')
	),
	include_directories: include_directories(
		join_paths(public_dir,'tier0'),
		join_paths(public_dir,'tier1'),
		video_services_src_dir
	),
	dependencies: [
		tier1.get_variable('tier1_dep'),
		tier0.get_variable('tier0_dep'),
		tier2.get_variable('tier2_dep'),
		tier3.get_variable('tier3_dep'),
		vstdlib.get_variable('vstdlib_dep'),
		source_base.get_variable('source_linux_shared_lib_dep'),
		dependency('vpx'),
		dependency('opus'),
		dependency('vorbis'),
		dependency('ogg'),
		webm.get_variable('webm_dep'),
		dependency('SDL2')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)

video_services_dep = declare_dependency(
	link_with: video_services_lib
)
