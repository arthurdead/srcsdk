project('theheist','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

client = get_option('client')

subproject('game')
if client == true
	subproject('vaudio_minimp3')
	subproject('vaudio_redirect')
	subproject('video_services')
endif