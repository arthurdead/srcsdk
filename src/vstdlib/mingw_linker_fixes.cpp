#ifdef _WIN32
#include <windows.h>
#endif

#include "tier0/platform.h"

#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef __MINGW32__
static bool vstdlib_loaded = false;

template <typename T>
static auto flatten_func_type(T input) -> void(*)(...)
{
	union {
		void(*ptr)(...);
		T func;
	};

	func = input;

	return ptr;
}

template <typename T>
static T get_func(HMODULE dll, const char *name)
{
	union {
		FARPROC ptr;
		T func;
	};

	ptr = GetProcAddress(dll, name);

	return func;
}

template <typename T>
static void get_func(T &func, HMODULE dll, const char *name)
{
	func = get_func<T>(dll, name);
}

extern "C" {
using _imp___ZN20CUniformRandomStreamC1Ev_t = void(CUniformRandomStream::*)();
DLL_EXPORT_ATTR _imp___ZN20CUniformRandomStreamC1Ev_t _imp___ZN20CUniformRandomStreamC1Ev = NULL;
using _imp___ZN20CUniformRandomStream9RandomIntEii_t = decltype(static_cast<int(CUniformRandomStream::*)(int,int)>(&CUniformRandomStream::RandomInt));
DLL_EXPORT_ATTR _imp___ZN20CUniformRandomStream9RandomIntEii_t _imp___ZN20CUniformRandomStream9RandomIntEii = NULL;
using _imp___ZN20CUniformRandomStream11RandomFloatEff_t = decltype(static_cast<float(CUniformRandomStream::*)(float,float)>(&CUniformRandomStream::RandomFloat));
DLL_EXPORT_ATTR _imp___ZN20CUniformRandomStream11RandomFloatEff_t _imp___ZN20CUniformRandomStream11RandomFloatEff = NULL;
using _imp___ZN20CUniformRandomStream7SetSeedEi_t = decltype(static_cast<void(CUniformRandomStream::*)(int)>(&CUniformRandomStream::SetSeed));
DLL_EXPORT_ATTR _imp___ZN20CUniformRandomStream7SetSeedEi_t _imp___ZN20CUniformRandomStream7SetSeedEi = NULL;

using _imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_t = void(CGaussianRandomStream::*)();
DLL_EXPORT_ATTR _imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_t _imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream = NULL;
using _imp___ZN21CGaussianRandomStream11RandomFloatEff_t = decltype(static_cast<float(CGaussianRandomStream::*)(float,float)>(&CGaussianRandomStream::RandomFloat));
DLL_EXPORT_ATTR _imp___ZN21CGaussianRandomStream11RandomFloatEff_t _imp___ZN21CGaussianRandomStream11RandomFloatEff = NULL;
using _imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_t = decltype(static_cast<void(CGaussianRandomStream::*)(IUniformRandomStream *)>(&CGaussianRandomStream::AttachToStream));
DLL_EXPORT_ATTR _imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_t _imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream = NULL;
}

[[gnu::constructor(0)]] static void load_vstdlib()
{
	if(vstdlib_loaded)
		return;

	HMODULE dll = GetModuleHandleA("vstdlib.dll");

	get_func(_imp___ZN20CUniformRandomStreamC1Ev, dll, "??0CUniformRandomStream@@QAE@XZ");
	get_func(_imp___ZN20CUniformRandomStream9RandomIntEii, dll, "?RandomInt@CUniformRandomStream@@UAEHHH@Z");
	get_func(_imp___ZN20CUniformRandomStream11RandomFloatEff, dll, "?RandomFloat@CUniformRandomStream@@UAEMMM@Z");
	get_func(_imp___ZN20CUniformRandomStream7SetSeedEi, dll, "?SetSeed@CUniformRandomStream@@UAEXH@Z");

	get_func(_imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream, dll, "??0CGaussianRandomStream@@QAE@PAVIUniformRandomStream@@@Z");
	get_func(_imp___ZN21CGaussianRandomStream11RandomFloatEff, dll, "?RandomFloat@CGaussianRandomStream@@QAEMMM@Z");
	get_func(_imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream, dll, "?AttachToStream@CGaussianRandomStream@@QAEXPAVIUniformRandomStream@@@Z");

	vstdlib_loaded = true;
}
#endif
