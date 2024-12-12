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

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "tier0/platform.h"
#include "tier1/utldict.h"

#ifdef __GNUC__
#define HACKMGR_THISCALL __attribute__((__thiscall__))
#else
#define HACKMGR_THISCALL __thiscall
#endif

inline void *align(void *ptr, size_t alignment) noexcept
{ return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) & ~(alignment - 1)); }
constexpr inline size_t align(size_t value, size_t alignment) noexcept
{ return (value & ~(alignment - 1)); }
constexpr inline uint64_t align_up(uint64_t value, size_t alignment) noexcept
{ return ((value + (alignment - 1)) & ~(alignment - 1)); }

class generic_class
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

class generic_class2
{
public:
	[[noreturn]] inline void generic_function()
	{ __builtin_trap(); }
private:
	generic_class2() = delete;
	~generic_class2() = delete;
	generic_class2(const generic_class2 &) = delete;
	generic_class2 &operator=(const generic_class2 &) = delete;
	generic_class2(generic_class2 &&) = delete;
	generic_class2 &operator=(generic_class2 &&) = delete;
};

class generic_single_class : public generic_class
{
public:
	[[noreturn]] inline void generic_function()
	{ __builtin_trap(); }
private:
	generic_single_class() = delete;
	~generic_single_class() = delete;
	generic_single_class(const generic_single_class &) = delete;
	generic_single_class &operator=(const generic_single_class &) = delete;
	generic_single_class(generic_single_class &&) = delete;
	generic_single_class &operator=(generic_single_class &&) = delete;
};

class generic_multiple_class : public generic_class, public generic_class2
{
public:
	[[noreturn]] inline void generic_function()
	{ __builtin_trap(); }
private:
	generic_multiple_class() = delete;
	~generic_multiple_class() = delete;
	generic_multiple_class(const generic_multiple_class &) = delete;
	generic_multiple_class &operator=(const generic_multiple_class &) = delete;
	generic_multiple_class(generic_multiple_class &&) = delete;
	generic_multiple_class &operator=(generic_multiple_class &&) = delete;
};

using generic_object_t = generic_class;
using generic_func_t = void(*)();
using generic_plain_mfp_t = void(HACKMGR_THISCALL *)(generic_class *);
using generic_single_mfp_t = void(generic_single_class::*)();
using generic_multiple_mfp_t = void(generic_multiple_class::*)();
using generic_vtable_t = generic_plain_mfp_t *;

struct page_info final
{
#ifdef __linux__
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
#else
	inline page_info(void *ptr, size_t len) noexcept
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		size_t pagesize{static_cast<size_t>(info.dwPageSize)};
		start = align(ptr, pagesize);
		void *end{align(static_cast<unsigned char *>(ptr) + len, pagesize)};
		if(start == end) {
			size = len;
		} else {
			size = ((reinterpret_cast<uintptr_t>(start) - reinterpret_cast<uintptr_t>(end)) - pagesize);
		}
	}

	inline void protect(unsigned long flags) noexcept
	{ unsigned long old; VirtualProtect(start, size, flags, &old); }
#endif

	inline page_info(const page_info &other)
		: start(other.start), size(other.size)
	{
	}
	inline page_info &operator=(const page_info &other)
	{
		start = other.start;
		size = other.size;
		return *this;
	}
	inline page_info(page_info &&other)
		: start(other.start), size(other.size)
	{
	}
	inline page_info &operator=(page_info &&other)
	{
		start = other.start;
		size = other.size;
		return *this;
	}

private:
	void *start;
	size_t size;

private:
	page_info() = delete;
};

#ifdef __GNUC__
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

namespace hackmgr::gcc
{
	inline __cxxabiv1::vtable_prefix *vtable_prefix_from_object(void *ptr) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#else
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		return reinterpret_cast<__cxxabiv1::vtable_prefix *>(reinterpret_cast<unsigned char *>(*reinterpret_cast<generic_vtable_t *>(ptr)) - __builtin_offsetof(__cxxabiv1::vtable_prefix, origin));
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

	inline generic_vtable_t vtable_from_object(void *ptr) noexcept
	{
		__cxxabiv1::vtable_prefix *prefix{vtable_prefix_from_object(ptr)};
		return vtable_from_prefix(prefix);
	}
}
#endif

#ifdef _WIN32
#if defined __GNUC__ && defined __GXX_RTTI
	#define _RTTI 1
	#define _RTTI_RELATIVE_TYPEINFO 0
#endif

#if __has_include(<rttidata.h>)
	#include <rttidata.h>
#endif

#if !defined _RTTI_RELATIVE_TYPEINFO || !defined _RTTI
	#error
#endif

namespace hackmgr::__msvc_cxxabi
{
	struct TypeDescriptor
	{
	#if defined(_WIN64) || defined(_RTTI)
		const void *pVFTable;
	#else
		unsigned long hash;
	#endif
		void *spare;
		char name[];
	};

	typedef struct TypeDescriptor TypeDescriptor;

	struct PMD
	{
		int mdisp;
		int pdisp;
		int vdisp;
	};

	typedef struct PMD PMD;

	struct _s_RTTIClassHierarchyDescriptor;
	typedef const _s_RTTIClassHierarchyDescriptor _RTTIClassHierarchyDescriptor;

	struct _s_RTTIBaseClassDescriptor
	{
	#if _RTTI_RELATIVE_TYPEINFO
		int pTypeDescriptor;
	#else
		TypeDescriptor *pTypeDescriptor;
	#endif
		unsigned long numContainedBases;
		PMD where;
		unsigned long attributes;
	#if _RTTI_RELATIVE_TYPEINFO
		int pClassDescriptor;
	#else
		_RTTIClassHierarchyDescriptor *pClassDescriptor;
	#endif
	};

	typedef const _s_RTTIBaseClassDescriptor _RTTIBaseClassDescriptor;

	struct _s_RTTIBaseClassArray
	{
	#if _RTTI_RELATIVE_TYPEINFO
		int arrayOfBaseClassDescriptors[];
	#else
		#ifdef __GNUC__
		_RTTIBaseClassDescriptor **arrayOfBaseClassDescriptors;
		#else
		_RTTIBaseClassDescriptor *arrayOfBaseClassDescriptors[];
		#endif
	#endif
	};

	typedef const _s_RTTIBaseClassArray _RTTIBaseClassArray;

	struct _s_RTTIClassHierarchyDescriptor
	{
		unsigned long signature;
		unsigned long attributes;
		unsigned long numBaseClasses;
	#if _RTTI_RELATIVE_TYPEINFO
		int pBaseClassArray;
	#else
		_RTTIBaseClassArray *pBaseClassArray;
	#endif
	};

	typedef const _s_RTTIClassHierarchyDescriptor _RTTIClassHierarchyDescriptor;

	struct _s_RTTICompleteObjectLocator
	{
		unsigned long signature;
		unsigned long offset;
		unsigned long cdOffset;
	#if _RTTI_RELATIVE_TYPEINFO
		int pTypeDescriptor;
		int pClassDescriptor;
		int pSelf;
	#else
		TypeDescriptor *pTypeDescriptor;
		_RTTIClassHierarchyDescriptor *pClassDescriptor;
		#if VERSP_WIN64
		const _s_RTTICompleteObjectLocator *pSelf;
		#endif
	#endif
	};

	typedef const _s_RTTICompleteObjectLocator _RTTICompleteObjectLocator;

#if __has_include(<rttidata.h>)
	static_assert(sizeof(TypeDescriptor) == sizeof(::TypeDescriptor));
	static_assert(alignof(TypeDescriptor) == alignof(::TypeDescriptor));

	static_assert(sizeof(PMD) == sizeof(::PMD));
	static_assert(alignof(PMD) == alignof(::PMD));

	static_assert(sizeof(_RTTIClassHierarchyDescriptor) == sizeof(::_RTTIClassHierarchyDescriptor));
	static_assert(alignof(_RTTIClassHierarchyDescriptor) == alignof(::_RTTIClassHierarchyDescriptor));

	static_assert(sizeof(_RTTIBaseClassDescriptor) == sizeof(::_RTTIBaseClassDescriptor));
	static_assert(alignof(_RTTIBaseClassDescriptor) == alignof(::_RTTIBaseClassDescriptor));

	static_assert(sizeof(_RTTIBaseClassArray) == sizeof(::_RTTIBaseClassArray));
	static_assert(alignof(_RTTIBaseClassArray) == alignof(::_RTTIBaseClassArray));

	static_assert(sizeof(_RTTICompleteObjectLocator) == sizeof(::_RTTICompleteObjectLocator));
	static_assert(alignof(_RTTICompleteObjectLocator) == alignof(::_RTTICompleteObjectLocator));
#endif
}

#if !__has_include(<rttidata.h>)
using hackmgr::__msvc_cxxabi::TypeDescriptor;
using hackmgr::__msvc_cxxabi::PMD;
using hackmgr::__msvc_cxxabi::_RTTIClassHierarchyDescriptor;
using hackmgr::__msvc_cxxabi::_RTTIBaseClassDescriptor;
using hackmgr::__msvc_cxxabi::_RTTIBaseClassArray;
using hackmgr::__msvc_cxxabi::_RTTICompleteObjectLocator;
#endif

namespace hackmgr::msvc
{
	inline _RTTICompleteObjectLocator *vtable_prefix_from_object(void *ptr) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#else
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		return static_cast<_RTTICompleteObjectLocator ***>(ptr)[0][-1];
	#ifdef __clang__
		#pragma clang diagnostic pop
	#else
		#pragma GCC diagnostic pop
	#endif
	}

	inline generic_vtable_t vtable_from_object(void *ptr) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
	#else
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wcast-align"
	#endif
		return *reinterpret_cast<generic_vtable_t *>(ptr);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#else
		#pragma GCC diagnostic pop
	#endif
	}
}
#endif

namespace hackmgr::compiler_native
{
#ifdef __GNUC__
	using hackmgr::gcc::vtable_prefix_from_object;
	using hackmgr::gcc::vtable_from_prefix;
	using hackmgr::gcc::vtable_from_object;
#else
	using hackmgr::msvc::vtable_prefix_from_object;
	using hackmgr::msvc::vtable_from_object;
#endif
}

#ifdef __GNUC__
namespace hackmgr::gcc
{
	using intmfp_t DECL_ALIGN(4) = uint64_t;
	using single_intmfp_t = intmfp_t;
	using multiple_intmfp_t = intmfp_t;
	using virtual_intmfp_t = intmfp_t;
	using unknown_intmfp_t = intmfp_t;
}
#endif

#ifdef _WIN32
namespace hackmgr::msvc
{
	using single_intmfp_t = uintptr_t;
	using multiple_intmfp_t DECL_ALIGN(4) = uint64_t;
	#ifdef __GNUC__
	using unknown_intmfp_t __attribute__((__vector_size__(16))) DECL_ALIGN(4) = uint32_t;
	#else
	#error
	#endif
}
#endif

namespace hackmgr::compiler_native
{
#ifdef __GNUC__
	using hackmgr::gcc::intmfp_t;
	using hackmgr::gcc::single_intmfp_t;
	using hackmgr::gcc::multiple_intmfp_t;
	using hackmgr::gcc::virtual_intmfp_t;
	using hackmgr::gcc::unknown_intmfp_t;
#else
	using hackmgr::msvc::single_intmfp_t;
	using hackmgr::msvc::multiple_intmfp_t;
#endif
}

static_assert(sizeof(&generic_single_class::generic_function) == sizeof(hackmgr::compiler_native::single_intmfp_t));
static_assert(alignof(decltype(&generic_single_class::generic_function)) == alignof(hackmgr::compiler_native::single_intmfp_t));

static_assert(sizeof(&generic_multiple_class::generic_function) == sizeof(hackmgr::compiler_native::multiple_intmfp_t));
static_assert(alignof(decltype(&generic_multiple_class::generic_function)) == alignof(hackmgr::compiler_native::multiple_intmfp_t));

#ifdef __GNUC__
namespace hackmgr::gcc
{
	template <typename R, typename C, typename ...Args>
	struct DECL_ALIGN(alignof(intmfp_t)) mfp_internal_t
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

		constexpr inline mfp_internal_t(R(HACKMGR_THISCALL *addr_)(C *, Args...)) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		constexpr inline mfp_internal_t(R(HACKMGR_THISCALL *addr_)(C *, Args...), ptrdiff_t adjustor_) noexcept
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

		constexpr inline mfp_internal_t &operator=(R(HACKMGR_THISCALL *addr_)(C *, Args...)) noexcept
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

		union {
			intmfp_t value;
			struct {
				R(HACKMGR_THISCALL *addr)(C *, Args...);
				ptrdiff_t adjustor;
			};
			R(C::*func)(Args...) {nullptr};
		};
	};

	template <typename R, typename C, typename ...Args>
	using single_mfp_internal_t = mfp_internal_t<R, C, Args...>;

	template <typename R, typename C, typename ...Args>
	using multiple_mfp_internal_t = mfp_internal_t<R, C, Args...>;

	template <typename R, typename C, typename ...Args>
	using virtual_mfp_internal_t = mfp_internal_t<R, C, Args...>;

	template <typename R, typename C, typename ...Args>
	using unknown_mfp_internal_t = mfp_internal_t<R, C, Args...>;

	using generic_single_internal_mfp_t = single_mfp_internal_t<void, generic_single_class>;
	using generic_multiple_internal_mfp_t = single_mfp_internal_t<void, generic_multiple_class>;
}
#endif

#ifdef _WIN32
namespace hackmgr::msvc
{
	template <typename R, typename C, typename ...Args>
	struct DECL_ALIGN(alignof(single_intmfp_t)) single_mfp_internal_t
	{
		constexpr single_mfp_internal_t() noexcept = default;

		constexpr inline single_mfp_internal_t(std::nullptr_t) noexcept
			: func{nullptr}
		{
		}

		constexpr inline single_mfp_internal_t(single_intmfp_t value_) noexcept
			: value{value_}
		{
		}

		constexpr inline single_mfp_internal_t(R(C::*func_)(Args...)) noexcept
			: func{func_}
		{
		}

		constexpr inline single_mfp_internal_t(R(HACKMGR_THISCALL *addr_)(C *, Args...)) noexcept
			: addr{addr_}
		{
		}

		constexpr inline single_mfp_internal_t &operator=(single_intmfp_t value_) noexcept
		{
			value = value_;
			return *this;
		}

		constexpr inline single_mfp_internal_t &operator=(R(C::*func_)(Args...)) noexcept
		{
			func = func_;
			return *this;
		}

		constexpr inline single_mfp_internal_t &operator=(R(HACKMGR_THISCALL *addr_)(C *, Args...)) noexcept
		{
			addr = addr_;
			return *this;
		}

		constexpr single_mfp_internal_t(single_mfp_internal_t &&) noexcept = default;
		constexpr single_mfp_internal_t &operator=(single_mfp_internal_t &&) noexcept = default;
		constexpr single_mfp_internal_t(const single_mfp_internal_t &) noexcept = default;
		constexpr single_mfp_internal_t &operator=(const single_mfp_internal_t &) noexcept = default;

		constexpr inline operator bool() const noexcept
		{ return addr; }
		constexpr inline bool operator!() const noexcept
		{ return !addr; }

		union {
			single_intmfp_t value;
			struct {
				R(HACKMGR_THISCALL *addr)(C *, Args...);
			};
			R(C::*func)(Args...) {nullptr};
		};
	};

	template <typename R, typename C, typename ...Args>
	struct DECL_ALIGN(alignof(multiple_intmfp_t)) multiple_mfp_internal_t
	{
		constexpr multiple_mfp_internal_t() noexcept = default;

		constexpr inline multiple_mfp_internal_t(std::nullptr_t) noexcept
			: func{nullptr}
		{
		}

		constexpr inline multiple_mfp_internal_t(multiple_intmfp_t value_) noexcept
			: value{value_}
		{
		}

		constexpr inline multiple_mfp_internal_t(R(C::*func_)(Args...)) noexcept
			: func{func_}
		{
		}

		constexpr inline multiple_mfp_internal_t(R(HACKMGR_THISCALL *addr_)(C *, Args...)) noexcept
			: addr{addr_}, adjustor{0}
		{
		}

		constexpr inline multiple_mfp_internal_t(R(HACKMGR_THISCALL *addr_)(C *, Args...), ptrdiff_t adjustor_) noexcept
			: addr{addr_}, adjustor{adjustor_}
		{
		}

		constexpr inline multiple_mfp_internal_t &operator=(multiple_intmfp_t value_) noexcept
		{
			value = value_;
			return *this;
		}

		constexpr inline multiple_mfp_internal_t &operator=(R(C::*func_)(Args...)) noexcept
		{
			func = func_;
			return *this;
		}

		constexpr inline multiple_mfp_internal_t &operator=(R(HACKMGR_THISCALL *addr_)(C *, Args...)) noexcept
		{
			addr = addr_;
			adjustor = 0;
			return *this;
		}

		constexpr multiple_mfp_internal_t(multiple_mfp_internal_t &&) noexcept = default;
		constexpr multiple_mfp_internal_t &operator=(multiple_mfp_internal_t &&) noexcept = default;
		constexpr multiple_mfp_internal_t(const multiple_mfp_internal_t &) noexcept = default;
		constexpr multiple_mfp_internal_t &operator=(const multiple_mfp_internal_t &) noexcept = default;

		constexpr inline operator bool() const noexcept
		{ return addr; }
		constexpr inline bool operator!() const noexcept
		{ return !addr; }

		union {
			multiple_intmfp_t value;
			struct {
				R(HACKMGR_THISCALL *addr)(C *, Args...);
				ptrdiff_t adjustor;
			};
			R(C::*func)(Args...) {nullptr};
		};
	};

	using generic_single_internal_mfp_t = single_mfp_internal_t<void, generic_single_class>;
	using generic_multiple_internal_mfp_t = single_mfp_internal_t<void, generic_multiple_class>;
}
#endif

namespace hackmgr::compiler_native
{
#ifdef __GNUC__
	using hackmgr::gcc::single_mfp_internal_t;
	using hackmgr::gcc::multiple_mfp_internal_t;
	using hackmgr::gcc::virtual_mfp_internal_t;
	using hackmgr::gcc::unknown_mfp_internal_t;

	using hackmgr::gcc::generic_single_internal_mfp_t;
	using hackmgr::gcc::generic_multiple_internal_mfp_t;
#else
	using hackmgr::msvc::single_mfp_internal_t;
	using hackmgr::msvc::multiple_mfp_internal_t;

	using hackmgr::msvc::generic_single_internal_mfp_t;
	using hackmgr::msvc::generic_multiple_internal_mfp_t;
#endif
}

static_assert(sizeof(hackmgr::compiler_native::generic_single_internal_mfp_t) == sizeof(&generic_single_class::generic_function));
static_assert(alignof(hackmgr::compiler_native::generic_single_internal_mfp_t) == alignof(decltype(&generic_single_class::generic_function)));

static_assert(sizeof(hackmgr::compiler_native::generic_multiple_internal_mfp_t) == sizeof(&generic_multiple_class::generic_function));
static_assert(alignof(hackmgr::compiler_native::generic_multiple_internal_mfp_t) == alignof(decltype(&generic_multiple_class::generic_function)));

#ifdef __GNUC__
namespace hackmgr::gcc
{
	template <typename R, typename C, typename ...Args>
	inline size_t vfunc_index_single(R(C::*func)(Args...))
	{
		single_mfp_internal_t<R, C, Args...> internal{func};
		uintptr_t addr_value{reinterpret_cast<uintptr_t>(internal.addr)};
		if(!(addr_value & 1)) {
			return static_cast<size_t>(-1);
		} else {
			return ((addr_value-1) / sizeof(generic_plain_mfp_t));
		}
	}
}
#endif

namespace hackmgr::compiler_native
{
#ifdef __GNUC__
	using hackmgr::gcc::vfunc_index_single;
#else
	#error
#endif
}

#ifdef __GNUC__
namespace hackmgr::gcc
{
	template <typename T, unsigned char>
	struct func_from_vtable_t;

	template <typename R, typename C, typename ...Args>
	struct func_from_vtable_t<R(C::*)(Args...), 0>
	{
		static inline auto get(generic_plain_mfp_t func) -> R(C::*)(Args...)
		{
			single_mfp_internal_t<R, C, Args...> internal;
			internal.addr = reinterpret_cast<R(HACKMGR_THISCALL *)(C *, Args...)>(func);
			internal.adjustor = 0;
			return internal.func;
		}
	};

	template <typename R, typename C, typename ...Args>
	struct func_from_vtable_t<R(C::*)(Args...), 1>
	{
		static inline auto get(generic_plain_mfp_t func) -> R(C::*)(Args...)
		{
			multiple_mfp_internal_t<R, C, Args...> internal;
			internal.addr = reinterpret_cast<R(HACKMGR_THISCALL *)(C *, Args...)>(func);
			internal.adjustor = 0;
			return internal.func;
		}
	};

	template <typename R, typename C, typename ...Args>
	struct func_from_vtable_t<R(C::*)(Args...), 2>
	{
		static inline auto get(generic_plain_mfp_t func) -> R(C::*)(Args...)
		{
			virtual_mfp_internal_t<R, C, Args...> internal;
			internal.addr = reinterpret_cast<R(HACKMGR_THISCALL *)(C *, Args...)>(func);
			internal.adjustor = 0;
			return internal.func;
		}
	};

	template <typename R, typename C, typename ...Args>
	struct func_from_vtable_t<R(C::*)(Args...), 3>
	{
		static inline auto get(generic_plain_mfp_t func) -> R(C::*)(Args...)
		{
			unknown_mfp_internal_t<R, C, Args...> internal;
			internal.addr = reinterpret_cast<R(HACKMGR_THISCALL *)(C *, Args...)>(func);
			internal.adjustor = 0;
			return internal.func;
		}
	};

	template <typename T>
	inline auto func_from_single_vtable(generic_plain_mfp_t func)
	{
		return func_from_vtable_t<T, 0>::get(func);
	}

	template <typename T>
	inline auto func_from_multiple_vtable(generic_plain_mfp_t func)
	{
		return func_from_vtable_t<T, 1>::get(func);
	}

	template <typename T>
	inline auto func_from_virtual_vtable(generic_plain_mfp_t func)
	{
		return func_from_vtable_t<T, 2>::get(func);
	}

	template <typename T>
	inline auto func_from_unknown_vtable(generic_plain_mfp_t func)
	{
		return func_from_vtable_t<T, 3>::get(func);
	}
}
#endif

#ifdef _WIN32
namespace hackmgr::msvc
{
	template <typename T, unsigned char>
	struct func_from_vtable_t;

	template <typename R, typename C, typename ...Args>
	struct func_from_vtable_t<R(C::*)(Args...), 0>
	{
		static inline auto get(generic_plain_mfp_t func) -> R(C::*)(Args...)
		{
			single_mfp_internal_t<R, C, Args...> internal;
			internal.addr = reinterpret_cast<R(HACKMGR_THISCALL *)(C *, Args...)>(func);
			return internal.func;
		}
	};

	template <typename R, typename C, typename ...Args>
	struct func_from_vtable_t<R(C::*)(Args...), 1>
	{
		static inline auto get(generic_plain_mfp_t func) -> R(C::*)(Args...)
		{
			multiple_mfp_internal_t<R, C, Args...> internal;
			internal.addr = reinterpret_cast<R(HACKMGR_THISCALL *)(C *, Args...)>(func);
			return internal.func;
		}
	};

	template <typename T>
	inline auto func_from_single_vtable(generic_plain_mfp_t func)
	{
		return func_from_vtable_t<T, 0>::get(func);
	}

	template <typename T>
	inline auto func_from_multiple_vtable(generic_plain_mfp_t func)
	{
		return func_from_vtable_t<T, 1>::get(func);
	}
}
#endif

namespace hackmgr::target_runtime
{
#ifdef __linux__
	using namespace hackmgr::gcc;
#else
	using namespace hackmgr::msvc;
#endif
}

class KeyValues;

class AddressManager
{
public:
	AddressManager();
	~AddressManager();

	generic_func_t LookupFunction(const char *name);

private:
	void init();

	KeyValues *kv;
	bool initialized;

	CUtlDict<generic_func_t> funcs;
};
extern AddressManager addresses;

#endif
