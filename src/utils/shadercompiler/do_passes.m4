m4_divert(`-1')

m4_define(`M4_FORLOOP', `m4_ifelse(m4_eval(`($2) <= ($3)'), `1', `m4_pushdef(`$1')__$0_IMPL(`$1', m4_eval(`$2'), m4_eval(`$3'), `$4')m4_popdef(`$1')')')
m4_define(`__M4_FORLOOP_IMPL', `m4_define(`$1', `$2')$4`'m4_ifelse(`$2', `$3', `', `$0(`$1', m4_incr(`$2'), `$3', `$4')')')

m4_define(`MAX_SUPPORTED_PASSES', `20')

m4_divert`'m4_dnl

`#'define `MAX_SUPPORTED_PASSES' m4_defn(`MAX_SUPPORTED_PASSES')

template <size_t N>
struct do_passes_impl
{
	template <typename F>
	static shader_combo exec(typename shader_passes_t::const_iterator start_it, const shader_passes_t &passes, F &&func) = delete;
};

m4_define(`M4_DO_PASSES_IMPL', `

for(values[`$2'] = its[`$2']->low; values[`$2'] <= static_cast<ssize_t>(its[`$2']->high); ++values[`$2']) {
	{
		size_t tmp_number_buffer_len(number_buffer_len);

		for(;;) {
			char *num_begin(defines[`$2'].first+its[`$2']->name.length()+1);
			char *num_end(num_begin+tmp_number_buffer_len+1);

			std::to_chars_result tc(std::to_chars(num_begin, num_end, values[`$2']));
			if(tc.ec == std::errc::value_too_large) {
				++tmp_number_buffer_len;
				defines[`$2'].first = (char *)std::realloc(defines[`$2'].first, its[`$2']->name.length()+1+tmp_number_buffer_len+1);
			} else if(tc.ec == std::errc()) {
				const size_t num_len(tc.ptr - num_begin);
				defines[`$2'].second = its[$2]->name.length()+1+num_len;
				defines[`$2'].first[defines[`$2'].second] = static_cast<char>(0);
				break;
			} else {
				std::cout << "error while converthing number to string\n"sv;
				exit(EXIT_FAILURE);
			}
		}
	}

	m4_ifelse(m4_eval(`($2) < (($3)-1)'), `1', `M4_DO_PASSES_IMPL(its[$2], m4_incr($2), $3)', `

	size_t combo_hash(0);

	M4_FORLOOP(`M4_P', `0', m4_eval(`(($3)-1)'), `
	combo_hash ^= name_hashes[m4_defn(`M4_P')] + 0x9e3779b9 + (combo_hash<<6) + (combo_hash>>2);
	combo_hash ^= std::hash<size_t>()(values[m4_defn(`M4_P')]) + 0x9e3779b9 + (combo_hash<<6) + (combo_hash>>2);
	')

	func(combo, combo_hash, its[`$2']->type, defines);

	++combo.total_overall;

	if(its[`$2']->type == shader_pass_type::dynamic) {
		++combo.total_dynamic;
	} else {
		++combo.total_static;
	}
	')
}

')

M4_FORLOOP(`M4_I', `1', m4_defn(`MAX_SUPPORTED_PASSES'), `

template <>
struct do_passes_impl<m4_defn(`M4_I')>
{
	template <typename F>
	static shader_combo exec(typename shader_passes_t::const_iterator start_it, const shader_passes_t &passes, F &&func)
	{
		std::array<typename shader_passes_t::const_iterator, m4_defn(`M4_I')> its;
		std::array<ssize_t, m4_defn(`M4_I')> values;
		std::array<std::pair<char *, size_t>, m4_defn(`M4_I')> defines;
		std::array<size_t, m4_defn(`M4_I')> name_hashes;

		its[0] = start_it;

		M4_FORLOOP(`M4_J', `1', m4_eval(m4_defn(`M4_I')-1), `
		its[m4_defn(`M4_J')] = its[m4_eval(m4_defn(`M4_J')-1)];
		std::advance(its[m4_defn(`M4_J')], 1);
		')

		std::sort(std::begin(its), std::end(its),
			[](typename shader_passes_t::const_iterator rhs, typename shader_passes_t::const_iterator lhs) -> bool {
				return (rhs->total() > lhs->total());
			}
		);

		constexpr size_t number_buffer_len(5);

		M4_FORLOOP(`M4_K', `0', m4_eval(m4_defn(`M4_I')-1), `
		defines[m4_defn(`M4_K')].second = its[m4_defn(`M4_K')]->name.length()+1+number_buffer_len+1;
		defines[m4_defn(`M4_K')].first = (char *)malloc(defines[m4_defn(`M4_K')].second);
		std::strncpy(defines[m4_defn(`M4_K')].first, its[m4_defn(`M4_K')]->name.c_str(), its[m4_defn(`M4_K')]->name.length());
		defines[m4_defn(`M4_K')].first[its[m4_defn(`M4_K')]->name.length()] = "="[0];
		')

		M4_FORLOOP(`M4_K', `0', m4_eval(m4_defn(`M4_I')-1), `
		name_hashes[m4_defn(`M4_K')] = std::hash<std::string>{}(its[m4_defn(`M4_K')]->name);
		')

		shader_combo combo;

		combo.total_overall = 0;
		combo.total_dynamic = 0;
		combo.total_static = 0;

		M4_DO_PASSES_IMPL(`start_it', 0, m4_defn(`M4_I'))

		M4_FORLOOP(`M4_L', `0', m4_eval(m4_defn(`M4_I')-1), `
		std::free(defines[m4_defn(`M4_L')].first);
		')

		return combo;
	}
};

')

template <typename F>
static shader_combo do_passes(size_t N, typename shader_passes_t::const_iterator start_it, const shader_passes_t &passes, F &&func)
{
	switch(N) {
M4_FORLOOP(`M4_I', `1', m4_defn(`MAX_SUPPORTED_PASSES'), `
	case m4_defn(`M4_I'):
		return do_passes_impl<m4_defn(`M4_I')>::exec(start_it, passes, std::forward<F>(func));
')
	default:
		std::cout << "unsupported number of passes: "sv << N << '\n';
		exit(EXIT_FAILURE);
	}
}
