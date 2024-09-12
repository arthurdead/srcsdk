project('tier1', 'cpp', 'c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
build_libs = get_option('build_libs')

tier1_src_dir = join_paths(src_root_dir,'tier1')

source_base = subproject('source_base')

if build_libs
	tier1_lib = static_library('tier1',name_prefix:'',
		cpp_args: [
			'-DTIER1_STATIC_LIB',
			'-DLIBNAME=tier1'
		],
		sources: files(
			join_paths(tier1_src_dir,'utlbinaryblock.cpp'),
			join_paths(tier1_src_dir,'NetAdr.cpp'),
			join_paths(tier1_src_dir,'commandbuffer.cpp'),
			join_paths(tier1_src_dir,'strtools.cpp'),
			join_paths(tier1_src_dir,'stringpool.cpp'),
			join_paths(tier1_src_dir,'snappy.cpp'),
			join_paths(tier1_src_dir,'processor_detect_linux.cpp'),
			join_paths(tier1_src_dir,'tokenreader.cpp'),
			join_paths(tier1_src_dir,'strtools_unicode.cpp'),
			join_paths(tier1_src_dir,'snappy-stubs-internal.cpp'),
			join_paths(tier1_src_dir,'convar.cpp'),
			join_paths(tier1_src_dir,'kvpacker.cpp'),
			join_paths(tier1_src_dir,'snappy-sinksource.cpp'),
			join_paths(tier1_src_dir,'localize.cpp'),
			join_paths(tier1_src_dir,'tier1.cpp'),
			join_paths(tier1_src_dir,'checksum_crc.cpp'),
			join_paths(tier1_src_dir,'bitbuf.cpp'),
			join_paths(tier1_src_dir,'fileio.cpp'),
			join_paths(tier1_src_dir,'newbitbuf.cpp'),
			join_paths(tier1_src_dir,'diff.cpp'),
			join_paths(tier1_src_dir,'lzmaDecoder.cpp'),
			join_paths(tier1_src_dir,'sparsematrix.cpp'),
			join_paths(tier1_src_dir,'reliabletimer.cpp'),
			join_paths(tier1_src_dir,'utlstring.cpp'),
			join_paths(tier1_src_dir,'qsort_s.cpp'),
			join_paths(tier1_src_dir,'KeyValues.cpp'),
			join_paths(tier1_src_dir,'utlbufferutil.cpp'),
			join_paths(tier1_src_dir,'uniqueid.cpp'),
			join_paths(tier1_src_dir,'utlbuffer.cpp'),
			join_paths(tier1_src_dir,'interface.cpp'),
			join_paths(tier1_src_dir,'rangecheckedvar.cpp'),
			join_paths(tier1_src_dir,'characterset.cpp'),
			join_paths(tier1_src_dir,'utlsymbol.cpp'),
			join_paths(tier1_src_dir,'mempool.cpp'),
			join_paths(tier1_src_dir,'checksum_sha1.cpp'),
			join_paths(tier1_src_dir,'byteswap.cpp'),
			join_paths(tier1_src_dir,'datamanager.cpp'),
			join_paths(tier1_src_dir,'pathmatch.cpp'),
			join_paths(tier1_src_dir,'undiff.cpp'),
			join_paths(tier1_src_dir,'memstack.cpp'),
			join_paths(tier1_src_dir,'checksum_md5.cpp'),
			join_paths(tier1_src_dir,'generichash.cpp'),
			join_paths(tier1_src_dir,'splitstring.cpp'),
			join_paths(tier1_src_dir,'mapbase_matchers_base.cpp'),
			#join_paths(tier1_src_dir,'math_proxy.cpp'),
			join_paths(src_root_dir,'utils','lzma','C','LzmaDec.c')
		),
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier0'),
			join_paths(src_root_dir,'public','tier1')
		),
		dependencies: [
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: join_paths(src_root_dir,'lib','public','linux32')
	)

	tier1_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier1')
		),
		link_with: tier1_lib
	)
else
	tier1_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','tier1')
		),
		link_args: [
			'-l:tier1.a'
		]
	)
endif