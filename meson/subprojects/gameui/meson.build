project('gameui', 'cpp', default_options: [
	'cpp_std=gnu++20'
])

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_client = get_option('build_client')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
gameui_src_dir = join_paths(src_root_dir,'gameui')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
tier2 = subproject('tier2')
tier3 = subproject('tier3')
mathlib = subproject('mathlib')
vstdlib = subproject('vstdlib')
hackmgr = subproject('hackmgr')
vgui_controls = subproject('vgui_controls')
vtf = subproject('vtf')
matsys_controls = subproject('matsys_controls')
bitmap = subproject('bitmap')
steamworks = subproject('steamworks')

rmlui_game = subproject('rmlui_game')

if build_client
	shared_library('GameUI',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=GameUI',
			'-DMEMOVERRIDE_MODULE=GameUI',
			'-DGAMEUI_EXPORTS',
			'-DGAMEPADUI_DLL',
			'-DGAMECONTROLS_LIB',
			'-DSERVERBROWSER_EXPORTS',
		],
		sources: files(
			join_paths(gameui_src_dir,'gameconsole.cpp'),
			join_paths(gameui_src_dir,'loadingdialog.cpp'),
			join_paths(gameui_src_dir,'gameui.cpp'),
			join_paths(gameui_src_dir,'gamepadui_interface.cpp'),
			join_paths(gameui_src_dir,'gameuisystemmgr.cpp'),
			join_paths(gameui_src_dir,'serverbrowser.cpp'),
			join_paths(gameui_src_dir,'rungameengine.cpp'),
			join_paths(gameui_src_dir,'vguisystemmoduleloader.cpp'),
			join_paths(gameui_src_dir,'rmlui.cpp'),
			join_paths(gameui_src_dir,'rmlcontext.cpp'),
		),
		include_directories: include_directories(
			gameui_src_dir,
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(src_root_dir,'materialsystem')
		),
		dependencies: [
			tier1.get_variable('tier1_dep'),
			tier0.get_variable('tier0_dep'),
			tier2.get_variable('tier2_dep'),
			tier3.get_variable('tier3_dep'),
			mathlib.get_variable('mathlib_dep'),
			vstdlib.get_variable('vstdlib_dep'),
			steamworks.get_variable('steamworks_dep'),
			source_base.get_variable('source_shared_lib_dep'),
			hackmgr.get_variable('hackmgr_dep'),
			vgui_controls.get_variable('vgui_controls_dep'),
			matsys_controls.get_variable('matsys_controls_dep'),
			vtf.get_variable('vtf_dep'),
			bitmap.get_variable('bitmap_dep'),
			rmlui_game.get_variable('rmlui_game_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)

	install_data(
		join_paths(gameui_src_dir,'platformmenu.vdf'),
		install_dir: join_paths(game_dir,'bin'),
	)
endif
