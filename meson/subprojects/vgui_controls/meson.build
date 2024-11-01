project('vgui_controls', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
vgui_controls_src_dir = join_paths(src_root_dir,'vgui2','vgui_controls')

if build_libs
	vgui_controls_lib = static_library('vgui_controls',name_prefix:'',
		cpp_args: [
			'-DLIBNAME=vgui_controls'
		],
		sources: files(
			join_paths(vgui_controls_src_dir,'AnalogBar.cpp'),
			join_paths(vgui_controls_src_dir,'AnimatingImagePanel.cpp'),
			join_paths(vgui_controls_src_dir,'AnimationController.cpp'),
			join_paths(vgui_controls_src_dir,'BitmapImagePanel.cpp'),
			join_paths(vgui_controls_src_dir,'BuildFactoryHelper.cpp'),
			join_paths(vgui_controls_src_dir,'BuildGroup.cpp'),
			join_paths(vgui_controls_src_dir,'BuildModeDialog.cpp'),
			join_paths(vgui_controls_src_dir,'Button.cpp'),
			join_paths(vgui_controls_src_dir,'CheckButton.cpp'),
			join_paths(vgui_controls_src_dir,'CheckButtonList.cpp'),
			join_paths(vgui_controls_src_dir,'CircularProgressBar.cpp'),
			join_paths(vgui_controls_src_dir,'ComboBox.cpp'),
			join_paths(vgui_controls_src_dir,'consoledialog.cpp'),
			join_paths(vgui_controls_src_dir,'ControllerMap.cpp'),
			join_paths(vgui_controls_src_dir,'controls.cpp'),
			join_paths(vgui_controls_src_dir,'cvartogglecheckbutton.cpp'),
			join_paths(vgui_controls_src_dir,'DirectorySelectDialog.cpp'),
			join_paths(vgui_controls_src_dir,'Divider.cpp'),
			join_paths(vgui_controls_src_dir,'EditablePanel.cpp'),
			join_paths(vgui_controls_src_dir,'ExpandButton.cpp'),
			join_paths(vgui_controls_src_dir,'FileOpenDialog.cpp'),
			join_paths(vgui_controls_src_dir,'FileOpenStateMachine.cpp'),
			join_paths(vgui_controls_src_dir,'FocusNavGroup.cpp'),
			join_paths(vgui_controls_src_dir,'Frame.cpp'),
			join_paths(vgui_controls_src_dir,'GraphPanel.cpp'),
			join_paths(vgui_controls_src_dir,'HTML.cpp'),
			join_paths(vgui_controls_src_dir,'Image.cpp'),
			join_paths(vgui_controls_src_dir,'ImageList.cpp'),
			join_paths(vgui_controls_src_dir,'ImagePanel.cpp'),
			join_paths(vgui_controls_src_dir,'InputDialog.cpp'),
			join_paths(vgui_controls_src_dir,'KeyBindingHelpDialog.cpp'),
			join_paths(vgui_controls_src_dir,'KeyBoardEditorDialog.cpp'),
			join_paths(vgui_controls_src_dir,'Label.cpp'),
			join_paths(vgui_controls_src_dir,'ListPanel.cpp'),
			join_paths(vgui_controls_src_dir,'ListViewPanel.cpp'),
			join_paths(vgui_controls_src_dir,'Menu.cpp'),
			join_paths(vgui_controls_src_dir,'MenuBar.cpp'),
			join_paths(vgui_controls_src_dir,'MenuButton.cpp'),
			join_paths(vgui_controls_src_dir,'MenuItem.cpp'),
			join_paths(vgui_controls_src_dir,'MessageBox.cpp'),
			join_paths(vgui_controls_src_dir,'MessageDialog.cpp'),
			join_paths(vgui_controls_src_dir,'Panel.cpp'),
			join_paths(vgui_controls_src_dir,'PanelListPanel.cpp'),
			join_paths(vgui_controls_src_dir,'ProgressBar.cpp'),
			join_paths(vgui_controls_src_dir,'ProgressBox.cpp'),
			join_paths(vgui_controls_src_dir,'PropertyDialog.cpp'),
			join_paths(vgui_controls_src_dir,'PropertyPage.cpp'),
			join_paths(vgui_controls_src_dir,'PropertySheet.cpp'),
			join_paths(vgui_controls_src_dir,'QueryBox.cpp'),
			join_paths(vgui_controls_src_dir,'RadioButton.cpp'),
			join_paths(vgui_controls_src_dir,'RichText.cpp'),
			join_paths(vgui_controls_src_dir,'RotatingProgressBar.cpp'),
			join_paths(vgui_controls_src_dir,'savedocumentquery.cpp'),
			join_paths(vgui_controls_src_dir,'ScalableImagePanel.cpp'),
			join_paths(vgui_controls_src_dir,'ScrollableEditablePanel.cpp'),
			join_paths(vgui_controls_src_dir,'ScrollBar.cpp'),
			join_paths(vgui_controls_src_dir,'ScrollBarSlider.cpp'),
			join_paths(vgui_controls_src_dir,'SectionedListPanel.cpp'),
			join_paths(vgui_controls_src_dir,'Slider.cpp'),
			join_paths(vgui_controls_src_dir,'Splitter.cpp'),
			join_paths(vgui_controls_src_dir,'subrectimage.cpp'),
			join_paths(vgui_controls_src_dir,'TextEntry.cpp'),
			join_paths(vgui_controls_src_dir,'TextImage.cpp'),
			join_paths(vgui_controls_src_dir,'ToggleButton.cpp'),
			join_paths(vgui_controls_src_dir,'Tooltip.cpp'),
			join_paths(vgui_controls_src_dir,'ToolWindow.cpp'),
			join_paths(vgui_controls_src_dir,'TreeView.cpp'),
			join_paths(vgui_controls_src_dir,'TreeViewListControl.cpp'),
			join_paths(vgui_controls_src_dir,'URLLabel.cpp'),
			join_paths(vgui_controls_src_dir,'WizardPanel.cpp'),
			join_paths(vgui_controls_src_dir,'WizardSubPanel.cpp'),
			join_paths(vgui_controls_src_dir,'gcc_linker_fixes.cpp'),
			join_paths(src_root_dir,'vgui2','src','vgui_key_translation.cpp'),
			join_paths(public_dir,'filesystem_helpers.cpp')
		),
		include_directories: include_directories(
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(public_dir,'vgui_controls')
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: source_base.get_variable('lib_folder')
	)

	vgui_controls_dep = declare_dependency(
		sources: files(
			join_paths(public_dir,'vgui_controls','vgui_controls.cpp')
		),
		include_directories: include_directories(
			join_paths(public_dir,'vgui_controls')
		),
		link_with: vgui_controls_lib
	)
else
	vgui_controls_dep = declare_dependency(
		sources: files(
			join_paths(public_dir,'vgui_controls','vgui_controls.cpp')
		),
		include_directories: include_directories(
			join_paths(public_dir,'vgui_controls')
		),
		link_args: [
			'-l:vgui_controls'+source_base.get_variable('lib_ext')
		]
	)
endif