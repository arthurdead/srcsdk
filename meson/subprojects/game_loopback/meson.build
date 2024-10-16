project('game_loopback', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
game_loopback_src_dir = join_paths(src_root_dir,'game_loopback')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
hackmgr = subproject('hackmgr')

if build_libs
	game_loopback_lib = shared_library('game_loopback',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=game_loopback',
			'-DMEMOVERRIDE_MODULE=game_loopback',
		],
		sources: files(
			join_paths(game_loopback_src_dir,'game_loopback.cpp'),
		),
		include_directories: include_directories(
			game_loopback_src_dir,
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(public_dir,'game_loopback')
		),
		dependencies: [
			tier1.get_variable('tier1_dep'),
			tier0.get_variable('tier0_dep'),
			source_base.get_variable('source_shared_lib_dep'),
			hackmgr.get_variable('hackmgr_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
