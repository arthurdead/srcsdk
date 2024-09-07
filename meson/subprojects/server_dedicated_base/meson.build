project('source_game','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

server_dedicated_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=server_srv',
		'-DDEDICATED',
		'-DSWDS'
	]
)
