project('gameui', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_client = get_option('build_client')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
gameui_src_dir = join_paths(src_root_dir,'gameui')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')
hackmgr = subproject('hackmgr')
vgui_controls = subproject('vgui_controls')
vtf = subproject('vtf')
matsys_controls = subproject('matsys_controls')
bitmap = subproject('bitmap')

if build_client
	shared_library('GameUI',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=GameUI',
			'-DGAMEUI_EXPORTS',
			'-DGAMEPADUI_DLL',
		],
		sources: files(
			join_paths(gameui_src_dir,'vgetlegacydata.cpp'),
			join_paths(gameui_src_dir,'vcontrolleroptionsbuttons.cpp'),
			join_paths(gameui_src_dir,'gameconsoledialog.cpp'),
			join_paths(gameui_src_dir,'vcontrolslistpanel.cpp'),
			join_paths(gameui_src_dir,'createmultiplayergamedialog.cpp'),
			join_paths(gameui_src_dir,'vachievements.cpp'),
			join_paths(gameui_src_dir,'vleaderboard.cpp'),
			join_paths(gameui_src_dir,'vmainmenu.cpp'),
			join_paths(gameui_src_dir,'vcloud.cpp'),
			join_paths(gameui_src_dir,'mousemessageforwardingpanel.cpp'),
			join_paths(gameui_src_dir,'logofile.cpp'),
			join_paths(gameui_src_dir,'createmultiplayergamebotpage.cpp'),
			join_paths(gameui_src_dir,'vaudiovideo.cpp'),
			join_paths(gameui_src_dir,'vaudio.cpp'),
			join_paths(gameui_src_dir,'vspinnercontrol.cpp'),
			join_paths(gameui_src_dir,'multiplayeradvanceddialog.cpp'),
			join_paths(gameui_src_dir,'vguisystemmoduleloader.cpp'),
			join_paths(gameui_src_dir,'gameconsole.cpp'),
			join_paths(gameui_src_dir,'optionssubvoice.cpp'),
			join_paths(gameui_src_dir,'uigamedata_invite.cpp'),
			join_paths(gameui_src_dir,'vpasswordentry.cpp'),
			join_paths(gameui_src_dir,'gamemodes.cpp'),
			join_paths(gameui_src_dir,'vkeyboard.cpp'),
			join_paths(gameui_src_dir,'vcustomcampaigns.cpp'),
			join_paths(gameui_src_dir,'basemodpanel.cpp'),
			join_paths(gameui_src_dir,'createmultiplayergameserverpage.cpp'),
			join_paths(gameui_src_dir,'vdropdownmenu.cpp'),
			join_paths(gameui_src_dir,'vaddonassociation.cpp'),
			join_paths(gameui_src_dir,'vsignindialog.cpp'),
			join_paths(gameui_src_dir,'loadingdialog.cpp'),
			join_paths(gameui_src_dir,'vingamechapterselect.cpp'),
			join_paths(gameui_src_dir,'cvarnegatecheckbutton.cpp'),
			join_paths(gameui_src_dir,'vfoundpublicgames.cpp'),
			join_paths(gameui_src_dir,'uiavatarimage.cpp'),
			join_paths(gameui_src_dir,'cvartextentry.cpp'),
			join_paths(gameui_src_dir,'vkeyboardmouse.cpp'),
			join_paths(gameui_src_dir,'basesavegamedialog.cpp'),
			join_paths(gameui_src_dir,'vslidercontrol.cpp'),
			join_paths(gameui_src_dir,'optionssubaudio.cpp'),
			join_paths(gameui_src_dir,'vgamelobby.cpp'),
			join_paths(gameui_src_dir,'backgroundmenubutton.cpp'),
			join_paths(gameui_src_dir,'commandcheckbutton.cpp'),
			join_paths(gameui_src_dir,'vgamelobbychat.cpp'),
			join_paths(gameui_src_dir,'optionssubmultiplayer.cpp'),
			join_paths(gameui_src_dir,'vvoteoptions.cpp'),
			join_paths(gameui_src_dir,'urlbutton.cpp'),
			join_paths(gameui_src_dir,'loadingtippanel.cpp'),
			join_paths(gameui_src_dir,'vcontrolleroptions.cpp'),
			join_paths(gameui_src_dir,'vmultiplayer.cpp'),
			join_paths(gameui_src_dir,'vgenericwaitscreen.cpp'),
			join_paths(gameui_src_dir,'playerlistdialog.cpp'),
			join_paths(gameui_src_dir,'optionsdialog.cpp'),
			join_paths(gameui_src_dir,'vdownloadcampaign.cpp'),
			join_paths(gameui_src_dir,'keytogglecheckbutton.cpp'),
			join_paths(gameui_src_dir,'uigamedata.cpp'),
			join_paths(gameui_src_dir,'optionssubkeyboard.cpp'),
			join_paths(gameui_src_dir,'rungameengine.cpp'),
			join_paths(gameui_src_dir,'vgamesettings.cpp'),
			join_paths(gameui_src_dir,'vattractscreen.cpp'),
			join_paths(gameui_src_dir,'cvarslider.cpp'),
			join_paths(gameui_src_dir,'vfoundgames.cpp'),
			join_paths(gameui_src_dir,'vfooterpanel.cpp'),
			join_paths(gameui_src_dir,'vloadingprogress.cpp'),
			join_paths(gameui_src_dir,'optionssubvideo.cpp'),
			join_paths(gameui_src_dir,'vingamedifficultyselect.cpp'),
			join_paths(gameui_src_dir,'vvideo.cpp'),
			join_paths(gameui_src_dir,'vtransitionscreen.cpp'),
			join_paths(gameui_src_dir,'vfoundgroupgames.cpp'),
			join_paths(gameui_src_dir,'vaddons.cpp'),
			join_paths(gameui_src_dir,'optionssubgame.cpp'),
			join_paths(gameui_src_dir,'uigamedata_storage.cpp'),
			join_paths(gameui_src_dir,'voptions.cpp'),
			join_paths(gameui_src_dir,'gameui_util.cpp'),
			join_paths(gameui_src_dir,'gameui_cvartogglecheckbutton.cpp'),
			join_paths(gameui_src_dir,'createmultiplayergamegameplaypage.cpp'),
			join_paths(gameui_src_dir,'customtabexplanationdialog.cpp'),
			join_paths(gameui_src_dir,'textentrybox.cpp'),
			join_paths(gameui_src_dir,'basemodframe.cpp'),
			join_paths(gameui_src_dir,'vquickjoingroups.cpp'),
			join_paths(gameui_src_dir,'contentcontroldialog.cpp'),
			join_paths(gameui_src_dir,'optionssubportal.cpp'),
			join_paths(gameui_src_dir,'vcontrolleroptionssticks.cpp'),
			join_paths(gameui_src_dir,'vingamekickplayerlist.cpp'),
			join_paths(gameui_src_dir,'vhybridbutton.cpp'),
			join_paths(gameui_src_dir,'vgenericpanellist.cpp'),
			join_paths(gameui_src_dir,'sys_utils.cpp'),
			join_paths(gameui_src_dir,'vmenubackground.cpp'),
			join_paths(gameui_src_dir,'vsteamcloudconfirmation.cpp'),
			join_paths(gameui_src_dir,'vgameoptions.cpp'),
			join_paths(gameui_src_dir,'gameui_interface.cpp'),
			join_paths(gameui_src_dir,'gamepadui_interface.cpp'),
			join_paths(gameui_src_dir,'optionssubmouse.cpp'),
			join_paths(gameui_src_dir,'vgenericconfirmation.cpp'),
			join_paths(gameui_src_dir,'vingamemainmenu.cpp'),
			join_paths(gameui_src_dir,'vdownloads.cpp'),
			join_paths(gameui_src_dir,'scriptobject.cpp'),
			join_paths(gameui_src_dir,'optionssubdifficulty.cpp'),
			join_paths(gameui_src_dir,'vflyoutmenu.cpp'),
			join_paths(gameui_src_dir,'labeledcommandcombobox.cpp'),
			join_paths(gameui_src_dir,'modinfo.cpp'),
			join_paths(gameui_src_dir,'vquickjoin.cpp'),
			join_paths(gameui_src_dir,'nb_button.cpp'),
			join_paths(gameui_src_dir,'nb_header_footer.cpp'),
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
			vstdlib.get_variable('vstdlib_dep'),
			source_base.get_variable('source_shared_lib_dep'),
			hackmgr.get_variable('hackmgr_dep'),
			vgui_controls.get_variable('vgui_controls_dep'),
			matsys_controls.get_variable('matsys_controls_dep'),
			vtf.get_variable('vtf_dep'),
			bitmap.get_variable('bitmap_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
