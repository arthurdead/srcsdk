project('game_shared_base','cpp','c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

source_base = subproject('source_base')
tier1 = subproject('tier1')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')
mathlib = subproject('mathlib')
raytrace = subproject('raytrace')
choreoobjects = subproject('choreoobjects')
responserules = subproject('responserules')
dmxloader = subproject('dmxloader')
particles = subproject('particles')
tier2 = subproject('tier2')
tier3 = subproject('tier3')
steamworks = subproject('steamworks')
game_loopback = subproject('game_loopback')
recastnavigation_game = subproject('recastnavigation_game')

public_dir = join_paths(src_root_dir,'public')
common_dir = join_paths(src_root_dir,'common')

shared_src_dir = join_paths(src_root_dir,'game','shared')

game_shared_base_dep = declare_dependency(
	compile_args: [
		
	],
	sources: files(
		join_paths(shared_src_dir,'default','weapon_parse_default.cpp'),
		join_paths(shared_src_dir,'default','gamemovement_default.cpp'),
		join_paths(shared_src_dir,'default','movedata_default.cpp'),
		join_paths(shared_src_dir,'default','usermessages_default.cpp'),

		join_paths(shared_src_dir,'achievementmgr.cpp'),
		join_paths(shared_src_dir,'activitylist.cpp'),
		join_paths(shared_src_dir,'ammodef.cpp'),
		join_paths(shared_src_dir,'animation.cpp'),
		join_paths(shared_src_dir,'baseachievement.cpp'),
		join_paths(shared_src_dir,'basecombatcharacter_shared.cpp'),
		join_paths(shared_src_dir,'basecombatweapon_shared.cpp'),
		join_paths(shared_src_dir,'baseentity_shared.cpp'),
		join_paths(shared_src_dir,'basegrenade_shared.cpp'),
		join_paths(shared_src_dir,'baseparticleentity.cpp'),
		join_paths(shared_src_dir,'baseplayer_shared.cpp'),
		join_paths(shared_src_dir,'baseprojectile.cpp'),
		join_paths(shared_src_dir,'baseviewmodel_shared.cpp'),
		join_paths(shared_src_dir,'beam_shared.cpp'),
		join_paths(shared_src_dir,'collisionproperty.cpp'),
		join_paths(shared_src_dir,'death_pose.cpp'),
		join_paths(shared_src_dir,'decals.cpp'),
		join_paths(shared_src_dir,'effect_dispatch_data.cpp'),
		join_paths(shared_src_dir,'ehandle.cpp'),
		join_paths(shared_src_dir,'entitylist_base.cpp'),
		join_paths(shared_src_dir,'EntityParticleTrail_Shared.cpp'),
		join_paths(shared_src_dir,'env_detail_controller.cpp'),
		join_paths(shared_src_dir,'env_wind_shared.cpp'),
		join_paths(shared_src_dir,'eventlist.cpp'),
		join_paths(shared_src_dir,'func_ladder.cpp'),
		join_paths(shared_src_dir,'gamemovement.cpp'),
		join_paths(shared_src_dir,'gamerules.cpp'),
		join_paths(shared_src_dir,'gamerules_register.cpp'),
		join_paths(shared_src_dir,'GameStats.cpp'),
		join_paths(shared_src_dir,'gamestringpool.cpp'),
		join_paths(shared_src_dir,'gamevars_shared.cpp'),
		join_paths(shared_src_dir,'hintmessage.cpp'),
		join_paths(shared_src_dir,'hintsystem.cpp'),
		join_paths(shared_src_dir,'igamesystem.cpp'),
		join_paths(shared_src_dir,'mapentities_shared.cpp'),
		join_paths(shared_src_dir,'movevars_shared.cpp'),
		join_paths(shared_src_dir,'obstacle_pushaway.cpp'),
		join_paths(shared_src_dir,'particle_parse.cpp'),
		join_paths(shared_src_dir,'particlesystemquery.cpp'),
		join_paths(shared_src_dir,'physics_main_shared.cpp'),
		join_paths(shared_src_dir,'physics_shared.cpp'),
		join_paths(shared_src_dir,'point_posecontroller.cpp'),
		join_paths(shared_src_dir,'precache_register.cpp'),
		join_paths(shared_src_dir,'predictableid.cpp'),
		join_paths(shared_src_dir,'props_shared.cpp'),
		join_paths(shared_src_dir,'querycache.cpp'),
		join_paths(shared_src_dir,'ragdoll_shared.cpp'),
		join_paths(shared_src_dir,'rope_helpers.cpp'),
		join_paths(shared_src_dir,'sceneentity_shared.cpp'),
		join_paths(shared_src_dir,'sequence_Transitioner.cpp'),
		join_paths(shared_src_dir,'simtimer.cpp'),
		join_paths(shared_src_dir,'SoundEmitterSystem.cpp'),
		join_paths(shared_src_dir,'soundenvelope.cpp'),
		join_paths(shared_src_dir,'Sprite.cpp'),
		join_paths(shared_src_dir,'studio_shared.cpp'),
		join_paths(shared_src_dir,'takedamageinfo.cpp'),
		#join_paths(shared_src_dir,'teamplay_gamerules.cpp'),
		join_paths(shared_src_dir,'usercmd.cpp'),
		join_paths(shared_src_dir,'util_shared.cpp'),
		join_paths(shared_src_dir,'vehicle_viewblend_shared.cpp'),
		join_paths(shared_src_dir,'weapon_parse.cpp'),
		join_paths(shared_src_dir,'weapon_proficiency.cpp'),
		join_paths(shared_src_dir,'mp_shareddefs.cpp'),
		join_paths(shared_src_dir,'sheetsimulator.cpp'),
		join_paths(shared_src_dir,'usermessages.cpp'),
		join_paths(shared_src_dir,'SpriteTrail.cpp'),
		join_paths(shared_src_dir,'predictioncopy.cpp'),
		#join_paths(shared_src_dir,'teamplayroundbased_gamerules.cpp'),
		#join_paths(shared_src_dir,'teamplay_round_timer.cpp'),
		join_paths(shared_src_dir,'script_intro_shared.cpp'),
		join_paths(shared_src_dir,'playeranimstate.cpp'),
		join_paths(shared_src_dir,'ai_criteria.cpp'),
		join_paths(shared_src_dir,'ai_responsesystem.cpp'),
		join_paths(shared_src_dir,'ai_speechconcept.cpp'),
		join_paths(shared_src_dir,'game_timescale_shared.cpp'),
		join_paths(shared_src_dir,'map_entity.cpp'),
		join_paths(shared_src_dir,'ai_waypoint.cpp'),
		join_paths(shared_src_dir,'mapbase','matchers.cpp'),
		join_paths(shared_src_dir,'recast','recast_file.cpp'),
		join_paths(shared_src_dir,'recast','recast_mesh.cpp'),
		join_paths(shared_src_dir,'recast','recast_mgr.cpp'),
		join_paths(shared_src_dir,'recast','recast_mgr_ent.cpp'),
		join_paths(shared_src_dir,'recast','recast_prunetool.cpp'),
		join_paths(shared_src_dir,'recast','recast_tilecache_helpers.cpp'),
		join_paths(shared_src_dir,'recast','fastlz','fastlz.c'),
		join_paths(shared_src_dir,'mapbase','GlobalStrings.cpp'),
		join_paths(shared_src_dir,'vphysics_sound.cpp'),
		join_paths(public_dir,'interval.cpp'),
		join_paths(public_dir,'SoundParametersInternal.cpp'),
		join_paths(public_dir,'bone_setup.cpp'),
		join_paths(public_dir,'bone_accessor.cpp'),
		join_paths(public_dir,'collisionutils.cpp'),
		join_paths(public_dir,'dt_shared.cpp'),
		join_paths(public_dir,'dt_utlvector_common.cpp'),
		join_paths(public_dir,'editor_sendcommand.cpp'),
		join_paths(public_dir,'filesystem_helpers.cpp'),
		join_paths(public_dir,'interpolatortypes.cpp'),
		join_paths(public_dir,'keyframe','keyframe.cpp'),
		join_paths(public_dir,'map_utils.cpp'),
		join_paths(public_dir,'networkvar.cpp'),
		join_paths(public_dir,'registry.cpp'),
		join_paths(public_dir,'rope_physics.cpp'),
		join_paths(public_dir,'scratchpad3d.cpp'),
		join_paths(public_dir,'ScratchPadUtils.cpp'),
		join_paths(public_dir,'simple_physics.cpp'),
		join_paths(public_dir,'stringregistry.cpp'),
		join_paths(public_dir,'studio.cpp'),
		join_paths(public_dir,'bone_setup.cpp'),
		join_paths(public_dir,'tools','bonelist.cpp'),
		join_paths(public_dir,'crtmemdebug.cpp'),
		join_paths(public_dir,'sentence.cpp'),
		join_paths(public_dir,'phonemeconverter.cpp'),
		join_paths(public_dir,'vallocator.cpp'),
		join_paths(public_dir,'closedcaptions.cpp'),
		join_paths(public_dir,'builddisp.cpp'),
		join_paths(public_dir,'disp_powerinfo.cpp'),
		join_paths(public_dir,'disp_common.cpp'),
		join_paths(common_dir,'language.cpp'),
		join_paths(common_dir,'randoverride.cpp'),
		join_paths(common_dir,'compiledcaptionswap.cpp'),
	),
	include_directories: include_directories(
		shared_src_dir,
	),
	dependencies: [
		source_base.get_variable('source_slop_dep'),
		source_base.get_variable('source_shared_lib_dep'),
		tier0.get_variable('tier0_dep'),
		tier1.get_variable('tier1_dep'),
		mathlib.get_variable('mathlib_dep'),
		raytrace.get_variable('raytrace_dep'),
		vstdlib.get_variable('vstdlib_dep'),
		choreoobjects.get_variable('choreoobjects_dep'),
		responserules.get_variable('responserules_runtime_dep'),
		dmxloader.get_variable('dmxloader_dep'),
		particles.get_variable('particles_dep'),
		tier2.get_variable('tier2_dep'),
		tier3.get_variable('tier3_dep'),
		steamworks.get_variable('steamworks_dep'),
		recastnavigation_game.get_variable('recastnavigation_game_dep'),
	]
)
