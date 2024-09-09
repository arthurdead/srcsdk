project('vphysics', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

error('cannot be used yet until hackmgr can patch the engine pointers')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')
mathlib = subproject('mathlib')

hackmgr = subproject('hackmgr')

public_dir = join_paths(src_root_dir,'public')
vphysics_src_dir = join_paths(src_root_dir,'vphysics_jolt','vphysics_jolt')

joltphysics_game = subproject('joltphysics_game')

if build_libs
	vphysics_lib = shared_library('vphysics',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=vphysics',
			'-DGAME_SDK2013'
		],
		sources: files(
			join_paths(vphysics_src_dir,'vjolt_controller_fluid.cpp'),
			join_paths(vphysics_src_dir,'vjolt_environment.cpp'),
			join_paths(vphysics_src_dir,'vjolt_collide_trace.cpp'),
			join_paths(vphysics_src_dir,'vjolt_keyvalues_schema.cpp'),
			join_paths(vphysics_src_dir,'vjolt_object.cpp'),
			join_paths(vphysics_src_dir,'vjolt_friction.cpp'),
			join_paths(vphysics_src_dir,'vjolt_parse.cpp'),
			join_paths(vphysics_src_dir,'vjolt_constraints.cpp'),
			join_paths(vphysics_src_dir,'vjolt_objectpairhash.cpp'),
			join_paths(vphysics_src_dir,'vjolt_debugrender.cpp'),
			join_paths(vphysics_src_dir,'vjolt_collide.cpp'),
			join_paths(vphysics_src_dir,'vjolt_interface.cpp'),
			join_paths(vphysics_src_dir,'vjolt_controller_vehicle.cpp'),
			join_paths(vphysics_src_dir,'vjolt_querymodel.cpp'),
			join_paths(vphysics_src_dir,'vjolt_controller_shadow.cpp'),
			join_paths(vphysics_src_dir,'vjolt_surfaceprops.cpp'),
			join_paths(vphysics_src_dir,'vjolt_controller_player.cpp'),
			join_paths(vphysics_src_dir,'vjolt_controller_motion.cpp'),
			join_paths(vphysics_src_dir,'cbase.cpp'),
		),
		include_directories: include_directories(
			vphysics_src_dir,
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(public_dir,'vphysics')
		),
		dependencies: [
			tier1.get_variable('tier1_dep'),
			tier0.get_variable('tier0_dep'),
			vstdlib.get_variable('vstdlib_dep'),
			mathlib.get_variable('mathlib_dep'),
			hackmgr.get_variable('hackmgr_dep'),
			source_base.get_variable('source_shared_lib_dep'),
			joltphysics_game.get_variable('joltphysics_game_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
