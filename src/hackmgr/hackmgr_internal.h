#ifndef HACKMGR_INTERNAL_H
#define HACKMGR_INTERNAL_H

#pragma once

#ifdef __x86_64__
#error
#endif

#ifdef __GNUC__
#include <cxxabi.h>
#endif

#ifdef __linux__
#include <sys/mman.h>
#endif

#include "tier0/platform.h"

#ifdef __GNUC__
#define __thiscall __attribute__((__thiscall__))
#endif

inline void *align(void *ptr, size_t alignment) noexcept
{ return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) & ~(alignment - 1)); }
constexpr inline size_t align(size_t value, size_t alignment) noexcept
{ return (value & ~(alignment - 1)); }
constexpr inline uint64_t align_up(uint64_t value, size_t alignment) noexcept
{ return ((value + (alignment - 1)) & ~(alignment - 1)); }

class generic_class final
{
public:
	[[noreturn]] inline void generic_function()
	{ __builtin_trap(); }
private:
	generic_class() = delete;
	~generic_class() = delete;
	generic_class(const generic_class &) = delete;
	generic_class &operator=(const generic_class &) = delete;
	generic_class(generic_class &&) = delete;
	generic_class &operator=(generic_class &&) = delete;
};

using generic_object_t = generic_class;
using generic_func_t = void(*)();
using generic_plain_mfp_t = void(__thiscall *)(generic_class *);
using generic_mfp_t = void(generic_class::*)();
using generic_vtable_t = generic_plain_mfp_t *;

#if defined __GNUC__ && defined __linux__
#if __has_include(<tinfo.h>)
	#include <tinfo.h>
#endif

namespace hackmgr::__cxxabi
{
#if __has_include(<tinfo.h>)
	#define HACKMGR_CXXABI_VTABLE_PREFIX_ALIGN alignas(__cxxabiv1::vtable_prefix)
#else
	#define HACKMGR_CXXABI_VTABLE_PREFIX_ALIGN
#endif

	struct HACKMGR_CXXABI_VTABLE_PREFIX_ALIGN vtable_prefix final
	{
		std::ptrdiff_t whole_object;
	#ifdef _GLIBCXX_VTABLE_PADDING
		std::ptrdiff_t padding1;
	#endif
		const __cxxabiv1::__class_type_info *whole_type;
	#ifdef _GLIBCXX_VTABLE_PADDING
		std::ptrdiff_t padding2;
	#endif
		const void *origin;
	};

#if __has_include(<tinfo.h>)
	static_assert(sizeof(vtable_prefix) == sizeof(__cxxabiv1::vtable_prefix));
	static_assert(alignof(vtable_prefix) == alignof(__cxxabiv1::vtable_prefix));
#endif
}

#if !__has_include(<tinfo.h>)
namespace __cxxabiv1
{
	using vtable_prefix = hackmgr::__cxxabi::vtable_prefix;
}
#endif

template <typename C>
inline __cxxabiv1::vtable_prefix *vtable_prefix_from_object(C *ptr) noexcept
{
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-align"
#endif
	return reinterpret_cast<__cxxabiv1::vtable_prefix *>(reinterpret_cast<unsigned char *>(*reinterpret_cast<generic_vtable_t *>(ptr)) - offsetof(__cxxabiv1::vtable_prefix, origin));
#ifdef __clang__
	#pragma clang diagnostic pop
#else
	#pragma GCC diagnostic pop
#endif
}

inline generic_vtable_t vtable_from_prefix(__cxxabiv1::vtable_prefix *prefix) noexcept
{
	return reinterpret_cast<generic_vtable_t>(const_cast<void **>(&prefix->origin));
}

template <typename C>
inline generic_vtable_t vtable_from_object(C *ptr) noexcept
{
	__cxxabiv1::vtable_prefix *prefix{vtable_prefix_from_object(ptr)};
	return vtable_from_prefix(prefix);
}

struct page_info final
{
	inline page_info(void *ptr, size_t len) noexcept
	{
		size_t pagesize{static_cast<size_t>(sysconf(_SC_PAGESIZE))};
		start = align(ptr, pagesize);
		void *end{align(static_cast<unsigned char *>(ptr) + len, pagesize)};
		if(start == end) {
			size = len;
		} else {
			size = ((reinterpret_cast<uintptr_t>(start) - reinterpret_cast<uintptr_t>(end)) - pagesize);
		}
	}

	inline void protect(int flags) noexcept
	{ mprotect(start, size, flags); }

private:
	void *start;
	size_t size;

private:
	page_info() = delete;
	page_info(const page_info &) = delete;
	page_info &operator=(const page_info &) = delete;
	page_info(page_info &&) = delete;
	page_info &operator=(page_info &&) = delete;
};

using intmfp_t = uint64_t;
#endif

static_assert(sizeof(&generic_class::generic_function) == sizeof(intmfp_t));
static_assert(alignof(&generic_class::generic_function) == alignof(intmfp_t));

template <typename R, typename C, typename ...Args>
union alignas(intmfp_t) mfp_internal_t
{
	constexpr mfp_internal_t() noexcept = default;

	constexpr inline mfp_internal_t(std::nullptr_t) noexcept
		: func{nullptr}
	{
	}

	constexpr inline mfp_internal_t(intmfp_t value_) noexcept
		: value{value_}
	{
	}

	constexpr inline mfp_internal_t(R(C::*func_)(Args...)) noexcept
		: func{func_}
	{
	}

	constexpr inline mfp_internal_t(R(__thiscall *addr_)(C *, Args...)) noexcept
		: addr{addr_}, adjustor{0}
	{
	}

	constexpr inline mfp_internal_t(R(__thiscall *addr_)(C *, Args...), size_t adjustor_) noexcept
		: addr{addr_}, adjustor{adjustor_}
	{
	}

	constexpr inline mfp_internal_t &operator=(intmfp_t value_) noexcept
	{
		value = value_;
		return *this;
	}

	constexpr inline mfp_internal_t &operator=(R(C::*func_)(Args...)) noexcept
	{
		func = func_;
		return *this;
	}

	constexpr inline mfp_internal_t &operator=(R(__thiscall *addr_)(C *, Args...)) noexcept
	{
		addr = addr_;
		adjustor = 0;
		return *this;
	}

	constexpr mfp_internal_t(mfp_internal_t &&) noexcept = default;
	constexpr mfp_internal_t &operator=(mfp_internal_t &&) noexcept = default;
	constexpr mfp_internal_t(const mfp_internal_t &) noexcept = default;
	constexpr mfp_internal_t &operator=(const mfp_internal_t &) noexcept = default;

	constexpr inline operator bool() const noexcept
	{ return addr; }
	constexpr inline bool operator!() const noexcept
	{ return !addr; }

	intmfp_t value;
	struct {
		R(__thiscall *addr)(C *, Args...);
		size_t adjustor;
	};
	R(C::*func)(Args...) {nullptr};
};

using generic_internal_mfp_t = mfp_internal_t<void, generic_class>;

static_assert(sizeof(generic_internal_mfp_t) == sizeof(&generic_class::generic_function));
static_assert(alignof(generic_internal_mfp_t) == alignof(&generic_class::generic_function));

template <typename R, typename C, typename ...Args>
inline size_t vfunc_index(R(C::*func)(Args...))
{
	mfp_internal_t<R, C, Args...> internal{func};
	uintptr_t addr_value{reinterpret_cast<uintptr_t>(internal.addr)};
	if(!(addr_value & 1)) {
		return static_cast<size_t>(-1);
	} else {
		return ((addr_value-1) / sizeof(generic_plain_mfp_t));
	}
}

#endif
