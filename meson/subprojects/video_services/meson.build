project('video_services', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
video_services_root_dir = join_paths(src_root_dir,'video_services')
video_services_src_dir = join_paths(video_services_root_dir,'video_services')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
tier2 = subproject('tier2')
tier3 = subproject('tier3')
vstdlib = subproject('vstdlib')
hackmgr = subproject('hackmgr')

vpx = dependency('vpx', required: false)
if not vpx.found()
	vpx = declare_dependency(
		include_directories: include_directories(
			join_paths(video_services_root_dir,'includes')
		),
		link_args: [
			'-L'+join_paths(video_services_root_dir,'lib','linux32'),
			'-lvpx'
		]
	)
endif

opus = dependency('opus', required: false)
if not opus.found()
	opus = declare_dependency(
		include_directories: include_directories(
			join_paths(video_services_root_dir,'includes')
		),
		link_args: [
			'-L'+join_paths(video_services_root_dir,'lib','linux32'),
			'-lopus'
		]
	)
endif

vorbis = dependency('vorbis', required: false)
if not vorbis.found()
	vorbis = declare_dependency(
		include_directories: include_directories(
			join_paths(video_services_root_dir,'includes')
		),
		link_args: [
			'-L'+join_paths(video_services_root_dir,'lib','linux32'),
			'-lvorbis'
		]
	)
endif

ogg = dependency('ogg', required: false)
if not ogg.found()
	ogg = declare_dependency(
		include_directories: include_directories(
			join_paths(video_services_root_dir,'includes')
		),
		link_args: [
			'-L'+join_paths(video_services_root_dir,'lib','linux32'),
			'-logg'
		]
	)
endif

sdl2 = dependency('SDL2', required: false)
if not sdl2.found()
	sdl2 = declare_dependency(
		include_directories: include_directories(
			join_paths(video_services_root_dir,'includes')
		),
		link_args: [
			'-L'+join_paths(video_services_root_dir,'lib','linux32'),
			'-lSDL2'
		]
	)
endif

cmake = import('cmake')

webm_opts = cmake.subproject_options()
webm_opts.add_cmake_defines({
	'CMAKE_POSITION_INDEPENDENT_CODE': true
})
webm = cmake.subproject('webm', options: webm_opts)

install_data(
	files(
		join_paths(video_services_root_dir,'lib','linux32','libSDL2.so'),
		join_paths(video_services_root_dir,'lib','linux32','libvpx.so'),
		join_paths(video_services_root_dir,'lib','linux32','libvpx.so.8.0.0'),
	),
	install_mode: 'rwxr-xr-x',
	install_dir: join_paths(game_dir,'bin')
)

video_services_lib = shared_library('video_services',name_prefix:'',
	cpp_args: [
		'-DDLLNAME=video_services',
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
		hackmgr.get_variable('hackmgr_dep'),
		source_base.get_variable('source_shared_lib_dep'),
		vpx,
		opus,
		vorbis,
		ogg,
		sdl2,
		webm.get_variable('webm_dep'),
	],
	install: true,
	install_dir: join_paths(game_dir,'bin'),
	install_rpath: '$ORIGIN'
)

video_services_dep = declare_dependency(
	link_with: video_services_lib
)
