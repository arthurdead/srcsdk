#ifndef __linux__
	#error
#endif

#ifdef __x86_64__
	#error
#endif

#include <vector>
#include <string>
#include <filesystem>
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <climits>
#include <unordered_set>
#include <charconv>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

struct path_pair
{
	std::filesystem::path linux_path;
	std::filesystem::path windows_path;
};

enum class shader_model : unsigned char
{
	ps2x,
	ps20b,
	ps30,
	vs20,
	vs30,
};

constexpr const char *opt_model[]{
	"ps_2_0",
	"ps_2_b",
	"ps_3_0",
	"vs_2_0",
	"vs_3_0",
};

constexpr const char *opt_model_define[]{
	"SHADER_MODEL_PS_2_0",
	"SHADER_MODEL_PS_2_B",
	"SHADER_MODEL_PS_3_0",
	"SHADER_MODEL_VS_2_0",
	"SHADER_MODEL_VS_3_0",
};

enum class shader_pass_type : unsigned char
{
	static_,
	dynamic,
};

struct shader_pass
{
	shader_pass() = delete;
	shader_pass(const shader_pass &) = default;
	shader_pass &operator=(const shader_pass &) = delete;
	shader_pass(shader_pass &&) = default;
	shader_pass &operator=(shader_pass &&) = delete;

	shader_pass(std::string &&name_, size_t low_, size_t high_, shader_pass_type type_);

	const std::string name;
	const size_t low;
	const size_t high;
	const shader_pass_type type;
	const size_t hash;
};

struct shader_combo;

struct hash_shader_pass
{
	static size_t hash(const shader_pass &pass)
	{
		size_t value(0);

		value ^= std::hash<std::string>()(pass.name) + 0x9e3779b9 + (value<<6) + (value>>2);
		value ^= std::hash<size_t>()(pass.low) + 0x9e3779b9 + (value<<6) + (value>>2);
		value ^= std::hash<size_t>()(pass.high) + 0x9e3779b9 + (value<<6) + (value>>2);
		value ^= std::hash<shader_pass_type>()(pass.type) + 0x9e3779b9 + (value<<6) + (value>>2);

		return value;
	}

	static size_t hash(const shader_combo &combo);

	size_t operator()(const shader_pass &pass) const
	{ return pass.hash; }

	size_t operator()(const shader_combo &combo) const;
};

shader_pass::shader_pass(std::string &&name_, size_t low_, size_t high_, shader_pass_type type_)
	: name(std::move(name_)), low(low_), high(high_), type(type_), hash(hash_shader_pass::hash(*this))
{
}

struct equal_shader_pass
{
	bool operator()(const shader_pass &rhs, const shader_pass &lhs) const
	{ return rhs.hash == lhs.hash; }

	bool operator()(const shader_combo &rhs, const shader_combo &lhs) const;
};

struct shader_combo
{
	shader_combo() = default;
	shader_combo(const shader_combo &) = default;
	shader_combo &operator=(const shader_combo &) = default;
	shader_combo(shader_combo &&) = default;
	shader_combo &operator=(shader_combo &&) = default;

	void calculate();

	std::unordered_set<shader_pass, hash_shader_pass, equal_shader_pass> passes;
	size_t num_combos;
	size_t hash;
};

size_t hash_shader_pass::hash(const shader_combo &combo)
{
	size_t value(0);

	for(const auto &it : combo.passes) {
		value ^= hash(it) + 0x9e3779b9 + (value<<6) + (value>>2);
	}

	return value;
}

size_t hash_shader_pass::operator()(const shader_combo &combo) const
{ return combo.hash; }

bool equal_shader_pass::operator()(const shader_combo &rhs, const shader_combo &lhs) const
{ return rhs.hash == lhs.hash; }

static std::unordered_set<shader_combo, hash_shader_pass, equal_shader_pass> combos;

void shader_combo::calculate()
{
	hash = 0;
	for(const auto &it : passes) {
		hash ^= it.hash + 0x9e3779b9 + (hash<<6) + (hash>>2);
	}

	num_combos = 1;
	for(const auto &it : passes) {
		num_combos *= ((it.high - it.low)+1);
	}

	if(!combos.contains(*this)) {
		combos.emplace(static_cast<const shader_combo &>(*this));
	}
}

struct shader_source : path_pair
{
	shader_model model;
	std::size_t output_index;
	std::vector<std::string> defines;
	shader_combo combo;
};

static void do_all_combos(const shader_source &shader, typename decltype(shader_combo::passes)::const_iterator skip_it)
{
	std::cout << shader.combo.num_combos << '\n';
}

static char winepath_buffer[PATH_MAX];
std::filesystem::path get_windows_path(const std::filesystem::path &wine_path, const std::filesystem::path &path)
{
	int stdout_pipes[2];
	if(pipe(stdout_pipes) == -1) {
		std::cout << "failed to pipe winepath stdout\n"sv;
		exit(EXIT_FAILURE);
	}

	pid_t wine_pid(vfork());
	if(wine_pid == 0) {
		if(dup2(stdout_pipes[1], STDOUT_FILENO) == -1) {
			_exit(EXIT_FAILURE);
		}

		close(stdout_pipes[0]);
		close(stdout_pipes[1]);

		const char *wine_argv[]{
			wine_path.c_str(),
			"winepath",
			"-w",
			path.c_str(),
			nullptr
		};
		execve(wine_path.c_str(), const_cast<char **>(wine_argv), environ);
		_exit(EXIT_FAILURE);
	} else if(wine_pid == -1) {
		std::cout << "failed to execute winepath\n"sv;
		exit(EXIT_FAILURE);
	}

	close(stdout_pipes[1]);

	char *const buffer_start(winepath_buffer);

	char *buffer_end(buffer_start + PATH_MAX);
	char *buffer_it(buffer_start);

	int status;
	while(waitpid(wine_pid, &status, WNOHANG) == 0) {
		if(read(stdout_pipes[0], buffer_it, 1) == 1 && buffer_it < buffer_end) {
			++buffer_it;
		}
	}

	close(stdout_pipes[0]);

	if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
		std::cout << "failed to execute winepath\n"sv;
		exit(EXIT_FAILURE);
	}

	buffer_end = buffer_start+((buffer_it-1)-buffer_start);

	std::string filename_str(buffer_start, buffer_end);

	return std::filesystem::path(std::move(filename_str));
}

static std::vector<path_pair> outputs;

static std::filesystem::path fxc_path;
static std::filesystem::path wine_path;

static size_t higest_define_count(0);

static std::vector<std::string> defines;

static std::vector<path_pair> includes;
static std::vector<shader_source> sources;

static bool werror(false);
static bool debug(false);
static bool strict(false);

static char opt_arg[]("/O?");
static size_t opt_level(3);

constexpr size_t fxc_argv_model_index(5);
constexpr size_t fxc_argv_model_define_index(7);
constexpr size_t fxc_argv_path_index(9);

static const char **fxc_argv;
static size_t fxc_argc(10);
static size_t fxc_argc_it(fxc_argc);

static void setup_fxc_argv()
{
	if(opt_level != static_cast<size_t>(-1)) {
		opt_arg[2] = static_cast<char>(static_cast<size_t>('0') + opt_level);
	} else {
		opt_arg[2] = 'd';
	}

	if(debug) {
		fxc_argc += 2;
	}

	if(werror) {
		++fxc_argc;
	}

	if(strict) {
		fxc_argc += 2;
	}

	fxc_argc += (higest_define_count * 2);
	fxc_argc += (defines.size() * 2);
	fxc_argc += (includes.size() * 2);

	fxc_argv = static_cast<const char **>(std::malloc(sizeof(char *) * fxc_argc));

	fxc_argv[0] = wine_path.c_str();
	fxc_argv[1] = fxc_path.c_str();
	fxc_argv[2] = "/nologo";
	fxc_argv[3] = opt_arg;
	fxc_argv[4] = "/T";
	fxc_argv[6] = "/D";
	fxc_argv[8] = "/Emain";

	for(const auto &it : defines) {
		fxc_argv[fxc_argc_it++] = "/D";
		fxc_argv[fxc_argc_it++] = it.c_str();
	}

	for(const auto &it : includes) {
		fxc_argv[fxc_argc_it++] = "/I";
		fxc_argv[fxc_argc_it++] = it.windows_path.c_str();
	}

	if(debug) {
		fxc_argv[fxc_argc_it++] = "/Zi";
		fxc_argv[fxc_argc_it++] = "/Zss";
	}

	if(werror) {
		fxc_argv[fxc_argc_it++] = "/WX";
	}

	if(strict) {
		fxc_argv[fxc_argc_it++] = "/Ges";
		fxc_argv[fxc_argc_it++] = "/Gis";
	}

	fxc_argv[fxc_argc_it++] = nullptr;
}

static void setup_fxc_argv_for_shader(const shader_source &info, size_t argc_it)
{
	for(const auto &it : info.defines) {
		fxc_argv[argc_it++] = "/D";
		fxc_argv[argc_it++] = it.c_str();
	}

	fxc_argv[argc_it++] = nullptr;
}

static char fxc_out_buffer[4096];
static char fxc_err_buffer[4096];
void compile_shader(const std::filesystem::path &wine_path, const std::filesystem::path &fxc_path, const shader_source &info)
{
	int stdout_pipes[2];
	if(pipe(stdout_pipes) == -1) {
		std::cout << "failed to pipe fxc stdout\n"sv;
		exit(EXIT_FAILURE);
	}

	int stderr_pipes[2];
	if(pipe(stderr_pipes) == -1) {
		std::cout << "failed to pipe fxc stderr"sv;
		exit(EXIT_FAILURE);
	}

	pid_t wine_pid(vfork());
	if(wine_pid == 0) {
		if(dup2(stdout_pipes[1], STDOUT_FILENO) == -1) {
			_exit(EXIT_FAILURE);
		}

		if(dup2(stderr_pipes[1], STDERR_FILENO) == -1) {
			_exit(EXIT_FAILURE);
		}

		close(stdout_pipes[0]);
		close(stdout_pipes[1]);

		close(stderr_pipes[0]);
		close(stderr_pipes[1]);

		fxc_argv[fxc_argv_model_index] = opt_model[static_cast<size_t>(info.model)];
		fxc_argv[fxc_argv_model_define_index] = opt_model_define[static_cast<size_t>(info.model)];
		fxc_argv[fxc_argv_path_index] = info.windows_path.c_str();

		setup_fxc_argv_for_shader(info, fxc_argc_it);

		std::cout << "executing "sv;
		for(size_t i(0); i < fxc_argc; ++i) {
			std::cout << fxc_argv[i] << ' ';
		}
		std::cout << '\n';
	#if 0
		execve(wine_path.c_str(), const_cast<char **>(fxc_argv), environ);
		_exit(EXIT_FAILURE);
	#else
		_exit(EXIT_SUCCESS);
	#endif
	} else if(wine_pid == -1) {
		std::cout << "failed to execute fxc\n"sv;
		exit(EXIT_FAILURE);
	}

	close(stdout_pipes[1]);
	close(stderr_pipes[1]);

	char *const out_buffer_start(fxc_out_buffer);
	char *const err_buffer_start(fxc_err_buffer);

	char *out_buffer_end(out_buffer_start + sizeof(fxc_out_buffer));
	char *out_buffer_it(out_buffer_start);

	char *err_buffer_end(err_buffer_start + sizeof(fxc_err_buffer));
	char *err_buffer_it(err_buffer_start);

	int status;
	while(waitpid(wine_pid, &status, WNOHANG) == 0) {
		if(read(stdout_pipes[0], out_buffer_it, 1) == 1 && out_buffer_it < out_buffer_end) {
			++out_buffer_it;
		}
		if(read(stderr_pipes[0], err_buffer_it, 1) == 1 && err_buffer_it < err_buffer_end) {
			++err_buffer_it;
		}
	}

	close(stdout_pipes[0]);
	close(stderr_pipes[0]);

	if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
		std::cout << "failed to execute fxc\n"sv;
		std::cout << out_buffer_start;
		std::cout << err_buffer_start;
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[], char *envp[])
{
	if(argc == 1) {
		std::cout << "no arguments\n"sv;
		return EXIT_FAILURE;
	}

	fxc_path = std::filesystem::current_path();

	fxc_path /= "winsdk_10.0.22621.0"sv;
	fxc_path /= "x86"sv;
	fxc_path /= "fxc.exe"sv;

	if(!std::filesystem::exists(fxc_path)) {
		std::cout << "fxc.exe not found in '"sv << fxc_path << "'\n"sv;
		return EXIT_FAILURE;
	}

	auto wine_env(getenv("WINE"));
	if(wine_env && wine_env[0] != '\0') {
		wine_path = wine_env;
	} else {
		wine_path = "/usr/bin/wine"s;
	}

	if(!std::filesystem::exists(wine_path)) {
		std::cout << "wine not found in '"sv << wine_path << "'\n"sv;
		return EXIT_FAILURE;
	}

	for(size_t i(1); i < static_cast<size_t>(argc); ++i) {
		if(std::strncmp(argv[i], "-I", 2) == 0) {
			if(*(argv[i]+2) == '\0') {
				if(i >= (argc-1)) {
					std::cout << "invalid argument syntax\n"sv;
					return EXIT_FAILURE;
				} else {
					std::filesystem::path linux_path(argv[i++]);
					if(std::filesystem::exists(linux_path)) {
						std::filesystem::path windows_path(get_windows_path(wine_path, linux_path));
						auto &inc(includes.emplace_back());
						inc.linux_path = std::move(linux_path);
						inc.windows_path = std::move(windows_path);
					} else {
						std::cout << "path '"sv << linux_path << "' does not exist\n"sv;
					}
				}
			} else {
				std::filesystem::path linux_path(argv[i]+2);
				if(std::filesystem::exists(linux_path)) {
					std::filesystem::path windows_path(get_windows_path(wine_path, linux_path));
					auto &inc(includes.emplace_back());
					inc.linux_path = std::move(linux_path);
					inc.windows_path = std::move(windows_path);
				} else {
					std::cout << "path '"sv << linux_path << "' does not exist\n"sv;
				}
			}
		} else if(std::strncmp(argv[i], "-o", 2) == 0) {
			if(*(argv[i]+2) == '\0') {
				if(i >= (argc-1)) {
					std::cout << "invalid argument syntax\n"sv;
					return EXIT_FAILURE;
				} else {
					std::filesystem::path linux_path(argv[i++]);
					if(std::filesystem::exists(linux_path)) {
						std::filesystem::path windows_path(get_windows_path(wine_path, linux_path));
						auto &out(outputs.emplace_back());
						out.linux_path = std::move(linux_path);
						out.windows_path = std::move(windows_path);
					} else {
						std::cout << "path '"sv << linux_path << "' does not exist\n"sv;
					}
				}
			} else {
				std::filesystem::path linux_path(argv[i]+2);
				if(std::filesystem::exists(linux_path)) {
					std::filesystem::path windows_path(get_windows_path(wine_path, linux_path));
					auto &out(outputs.emplace_back());
					out.linux_path = std::move(linux_path);
					out.windows_path = std::move(windows_path);
				} else {
					std::cout << "path '"sv << linux_path << "' does not exist\n"sv;
				}
			}
		} else if(std::strcmp(argv[i], "-W") == 0) {
			werror = true;
		} else if(std::strcmp(argv[i], "-G") == 0) {
			debug = true;
		} else if(std::strcmp(argv[i], "-S") == 0) {
			strict = true;
		} else if(std::strcmp(argv[i], "-D") == 0) {
			if(*(argv[i]+2) == '\0') {
				if(i >= (argc-1)) {
					std::cout << "invalid argument syntax\n"sv;
					return EXIT_FAILURE;
				} else {
					defines.emplace_back(argv[i++]);
				}
			} else {
				defines.emplace_back(argv[i]+2);
			}
		} else if(std::strncmp(argv[i], "-O", 2) == 0) {
			size_t len(std::strlen(argv[i]+2));
			if(len != 1) {
				std::cout << "invalid argument syntax\n"sv;
				return EXIT_FAILURE;
			}

			char c(*(argv[i]+2));
			if(c == 'd') {
				opt_level = static_cast<size_t>(-1);
			} else if(c >= '0' && c <= '3') {
				opt_level = (static_cast<size_t>(c) - static_cast<size_t>('0'));
			} else {
				std::cout << "invalid argument syntax\n"sv;
				return EXIT_FAILURE;
			}
		} else {
			std::filesystem::path linux_path(argv[i]);
			if(std::filesystem::exists(linux_path)) {
				std::filesystem::path ext(linux_path.extension());

				if(ext != ".fxc"sv) {
					std::cout << "file is not a shader: '"sv << linux_path << "'\n"sv;
					return EXIT_FAILURE;
				}

				std::filesystem::path filename(linux_path.filename());
				filename.replace_extension();

				const auto &filename_str(filename.native());

				shader_model model;

				if(filename_str.ends_with("_ps2x"sv)) {
					model = shader_model::ps2x;
				} else if(filename_str.ends_with("_ps20b"sv)) {
					model = shader_model::ps20b;
				} else if(filename_str.ends_with("_ps30"sv)) {
					model = shader_model::ps30;
				} else if(filename_str.ends_with("_vs20"sv)) {
					model = shader_model::vs20;
				} else if(filename_str.ends_with("_vs30"sv)) {
					model = shader_model::vs30;
				} else {
					std::cout << "invalid shader filename: "sv << filename_str << '\n';
					return EXIT_FAILURE;
				}

				std::filesystem::path windows_path(get_windows_path(wine_path, linux_path));

				auto &shader(sources.emplace_back());
				shader.model = model;
				shader.linux_path = std::move(linux_path);
				shader.windows_path = std::move(windows_path);

				if(outputs.empty()) {
					shader.output_index = static_cast<size_t>(-1);
				} else {
					shader.output_index = outputs.size()-1;
				}
			} else {
				std::cout << "path '"sv << linux_path << "' does not exist\n"sv;
			}
		}
	}

	if(sources.size() == 0) {
		std::cout << "no files\n"sv;
		return EXIT_FAILURE;
	}

	setup_fxc_argv();

	for(auto &it : sources) {
		auto fd(open(it.linux_path.c_str(), O_RDONLY));
		if(fd == -1) {
			const char *err(strerror(errno));
			std::cout << "failed to open '"sv << it.linux_path << "': "sv << err << '\n';
			return EXIT_FAILURE;
		}

		size_t line(1);
		size_t column(0);

		auto skip_whitespace = [fd,&column]() -> ssize_t {
			char c;
			for(;;) {
				ssize_t ret(read(fd, &c, 1));
				if(ret == 0) {
					return 0;
				} else if(ret == -1) {
					return -1;
				}

				if(c != ' ' && c != '\t') {
					lseek(fd, -1, SEEK_CUR);
					break;
				}

				++column;
			}

			return 1;
		};

		auto skip_newline = [fd,&line,&column]() -> ssize_t {
			char c;

			for(;;) {
				ssize_t ret(read(fd, &c, 1));
				if(ret == 0) {
					return 0;
				} else if(ret == -1) {
					return -1;
				}

				if(c == '\n') {
					break;
				}
			}

			++line;
			column = 0;

			return 1;
		};

		auto skip_emptyline = [fd,&line,&column]() -> ssize_t {
			char c;

			for(;;) {
				ssize_t ret(read(fd, &c, 1));
				if(ret == 0) {
					return 0;
				} else if(ret == -1) {
					return -1;
				}

				if(c == '\n') {
					++line;
					column = 0;
				} else if(c == ' ' || c == '\t') {
					++column;
				} else {
					lseek(fd, -1, SEEK_CUR);
					break;
				}
			}

			return 1;
		};

		auto read_identifier = [fd,&column](std::string &str) -> ssize_t {
			str.reserve(5);

			char c;
			ssize_t ret(read(fd, &c, 1));
			if(ret == 0) {
				return 0;
			} else if(ret == -1) {
				return -1;
			}

			if(!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && (c != '_')) {
				lseek(fd, -1, SEEK_CUR);
				return 2;
			}

			str += c;

			for(;;) {
				ret = read(fd, &c, 1);
				if(ret == 0) {
					return 0;
				} else if(ret == -1) {
					return -1;
				}

				if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_')) {
					str += c;
				} else {
					lseek(fd, -1, SEEK_CUR);
					break;
				}
			}

			column += str.size();

			return 1;
		};

		auto read_number = [fd,&column](ssize_t &num, std::string &str) -> ssize_t {
			str.reserve(5);

			char c;
			ssize_t ret(read(fd, &c, 1));
			if(ret == 0) {
				return 0;
			} else if(ret == -1) {
				return -1;
			}

			if(!(c >= '0' && c <= '9') && (c != '-')) {
				lseek(fd, -1, SEEK_CUR);
				return 2;
			}

			str += c;

			for(;;) {
				ret = read(fd, &c, 1);
				if(ret == 0) {
					return 0;
				} else if(ret == -1) {
					return -1;
				}

				if(c >= '0' && c <= '9') {
					str += c;
				} else {
					lseek(fd, -1, SEEK_CUR);
					break;
				}
			}

			column += str.size();

			const char *end(str.c_str()+str.length());

			std::from_chars_result fc_res(std::from_chars(str.c_str(), end, num, 10));

			if(fc_res.ec != std::errc() || fc_res.ptr != end) {
				return 3;
			}

			return 1;
		};

		ssize_t ret;

	#define SKIP_WHITESPACE_WRAP \
		ret = skip_whitespace(); \
		if(ret == 0) { \
			std::cout << it.linux_path << ':' << line << ':' << column << ": unexpected EOF\n"sv; \
			break; \
		} else if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		}

	#define SKIP_EMPTYSPACE_WRAP \
		ret = skip_whitespace(); \
		if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		}

	#define SKIP_NEWLINE_WRAP \
		ret = skip_newline(); \
		if(ret == 0) { \
			break; \
		} else if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		}

	#define SKIP_EMPTYLINE_WRAP \
		ret = skip_emptyline(); \
		if(ret == 0) { \
			break; \
		} else if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		}

	#define READ_WRAP(buf,len) \
		ret = read(fd, (buf), (len)); \
		if(ret == 0) { \
			std::cout << it.linux_path << ':' << line << ':' << column << ": unexpected EOF\n"sv; \
			break; \
		} else if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		} \
		column += ret;

	#define READ_ID_WRAP(str) \
		ret = read_identifier((str)); \
		if(ret == 0) { \
			std::cout << it.linux_path << ':' << line << ':' << column << ": unexpected EOF\n"sv; \
			break; \
		} else if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		}

	#define READ_NUM_WRAP(num,str) \
		ret = read_number((num),(str)); \
		if(ret == 0) { \
			std::cout << it.linux_path << ':' << line << ':' << column << ": unexpected EOF\n"sv; \
			break; \
		} else if(ret == -1) { \
			const char *err(strerror(errno)); \
			std::cout << "error while reading '"sv << it.linux_path << "': "sv << err << '\n'; \
			return EXIT_FAILURE; \
		}

		for(;;) {
			SKIP_EMPTYLINE_WRAP

			char cmt[3];
			cmt[2] = '\0';
			READ_WRAP(cmt, 2)

			if(std::strcmp(cmt, "//") == 0) {
				SKIP_WHITESPACE_WRAP
			} else {
				SKIP_NEWLINE_WRAP
				continue;
			}

			char directive[9];
			directive[8] = '\0';
			READ_WRAP(directive, 8)

			enum parse_type : unsigned char
			{
				static_pass = static_cast<unsigned char>(shader_pass_type::static_),
				dynamic_pass = static_cast<unsigned char>(shader_pass_type::dynamic),
				skip,
				centroid,
			};

			parse_type type;

			if(std::strncmp(directive, "STATIC", 6) == 0) {
				lseek(fd, -2, SEEK_CUR);
				directive[6] = '\0';

				type = parse_type::static_pass;
			} else if(std::strncmp(directive, "DYNAMIC", 7) == 0) {
				lseek(fd, -1, SEEK_CUR);
				directive[7] = '\0';

				type = parse_type::dynamic_pass;
			} else if(std::strncmp(directive, "SKIP", 4) == 0) {
				lseek(fd, -4, SEEK_CUR);
				directive[4] = '\0';

				type = parse_type::skip;
			} else if(std::strcmp(directive, "CENTROID") == 0) {
				type = parse_type::centroid;
			} else {
				SKIP_NEWLINE_WRAP
				continue;
			}

			SKIP_WHITESPACE_WRAP

			char colon;
			READ_WRAP(&colon, 1)

			if(colon == ':') {
				SKIP_WHITESPACE_WRAP
			} else {
				lseek(fd, -1, SEEK_CUR);
			}

			if(type == parse_type::static_pass || type == parse_type::dynamic_pass) {
				std::string_view type_str((type == parse_type::static_pass) ? "static"sv : "dynamic"sv);

				char quote;
				READ_WRAP(&quote, 1)

				if(quote == '"') {
					SKIP_WHITESPACE_WRAP
				} else {
					lseek(fd, -1, SEEK_CUR);
				}

				std::string pass_name;
				READ_ID_WRAP(pass_name)

				if(ret == 2) {
					std::cout << it.linux_path << ':' << line << ':' << column << ": failed to read "sv << type_str << " pass name\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				}

				if(std::find_if(it.combo.passes.cbegin(), it.combo.passes.cend(),
				[pass_name = std::string_view(pass_name)](const shader_pass &pass) -> bool {
					return (pass.name == pass_name);
				}) != it.combo.passes.cend()) {
					std::cout << it.linux_path << ':' << line << ':' << column << ": "sv << type_str << " pass '"sv << pass_name << "' was already registered, line will be ignored\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				}

				SKIP_WHITESPACE_WRAP

				if(quote == '"') {
					READ_WRAP(&quote, 1)

					if(quote == '"') {
						SKIP_WHITESPACE_WRAP
					} else {
						std::cout << it.linux_path << ':' << line << ':' << column << ": "sv << type_str << " pass '"sv << pass_name << "' has unclosed quote, pass will be discarted\n"sv;
						SKIP_NEWLINE_WRAP
						continue;
					}
				}

				READ_WRAP(&quote, 1)

				if(quote == '"') {
					SKIP_WHITESPACE_WRAP
				} else {
					lseek(fd, -1, SEEK_CUR);
				}

				ssize_t low; std::string low_str;
				READ_NUM_WRAP(low, low_str)

				if(ret == 2) {
					std::cout << it.linux_path << ':' << line << ':' << column << ": "sv << type_str << " pass '"sv << pass_name << "' low value doesn't start with a number, pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				} else if(ret == 3) {
					std::cout << it.linux_path << ':' << line << ':' << column << ": failed to read "sv << type_str << " pass '"sv << pass_name << "' low value, pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				}

				SKIP_WHITESPACE_WRAP

				READ_WRAP(cmt, 2)

				if(std::strcmp(cmt, "..") == 0) {
					SKIP_WHITESPACE_WRAP
				} else {
					size_t len(std::strlen(cmt));
					std::cout << it.linux_path << ':' << line << '-' << (column-len) << ": "sv << type_str << " pass '"sv << pass_name << "' expected '..' but found '"sv << std::string_view(cmt, len) << "', pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				}

				ssize_t hi; std::string hi_str;
				READ_NUM_WRAP(hi, hi_str)

				if(ret == 2) {
					std::cout << it.linux_path << ':' << line << ':' << column << ": "sv << type_str << " pass '"sv << pass_name << "' high value doesn't start with a number, pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				} else if(ret == 3) {
					std::cout << it.linux_path << ':' << line << ':' << column << ": failed to read "sv << type_str << " pass '"sv << pass_name << "' high value, pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				}

				if(low > hi) {
					std::cout << it.linux_path << ": "sv << type_str << " pass '"sv << pass_name << "' low value is higher than high value, pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				} else if(hi < low) {
					std::cout << it.linux_path << ": "sv << type_str << " pass '"sv << pass_name << "' high value is lower than low value, pass will be discarted\n"sv;
					SKIP_NEWLINE_WRAP
					continue;
				} else if(hi == low) {
					std::cout << it.linux_path << ": "sv << type_str << " pass '"sv << pass_name << "' high value is equal to low value, pass will be converted to define\n"sv;

					std::string define(std::move(pass_name));
					define += '=';
					define += std::move(low_str);

					it.defines.emplace_back(std::move(define));

					if(it.defines.size() > higest_define_count) {
						higest_define_count = it.defines.size();
					}

					SKIP_NEWLINE_WRAP
					continue;
				}

				if(quote == '"') {
					SKIP_WHITESPACE_WRAP

					READ_WRAP(&quote, 1)

					if(quote == '"') {
						SKIP_EMPTYSPACE_WRAP
					} else {
						std::cout << it.linux_path << ':' << line << ':' << column << ": "sv << type_str << " pass '"sv << pass_name << "' has unclosed quote, pass will be discarted\n"sv;
						SKIP_NEWLINE_WRAP
						continue;
					}
				} else {
					SKIP_EMPTYSPACE_WRAP
				}

				shader_pass pass(std::move(pass_name), low, hi, static_cast<shader_pass_type>(type));
				it.combo.passes.emplace(std::move(pass));

				SKIP_NEWLINE_WRAP
			} else if(type == parse_type::skip) {
				std::cout << it.linux_path << ": skip directive is not supported yet\n"sv;

				SKIP_NEWLINE_WRAP
			} else if(type == parse_type::centroid) {
				std::cout << it.linux_path << ": centroid directive is not supported yet\n"sv;

				SKIP_NEWLINE_WRAP
			}
		}

		close(fd);
	}

	for(auto &it : sources) {
		it.combo.calculate();

		do_all_combos(it, it.combo.passes.cend());
	}

	return EXIT_SUCCESS;
}