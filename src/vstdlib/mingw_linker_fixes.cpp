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

using _imp___ZN20CUniformRandomStreamC1Ev_t = void(CUniformRandomStream::*)();
_imp___ZN20CUniformRandomStreamC1Ev_t _imp___ZN20CUniformRandomStreamC1Ev_ptr = NULL;
using _imp___ZN20CUniformRandomStream9RandomIntEii_t = decltype(static_cast<int(CUniformRandomStream::*)(int,int)>(&CUniformRandomStream::RandomInt));
_imp___ZN20CUniformRandomStream9RandomIntEii_t _imp___ZN20CUniformRandomStream9RandomIntEii_ptr = NULL;
using _imp___ZN20CUniformRandomStream11RandomFloatEff_t = decltype(static_cast<float(CUniformRandomStream::*)(float,float)>(&CUniformRandomStream::RandomFloat));
_imp___ZN20CUniformRandomStream11RandomFloatEff_t _imp___ZN20CUniformRandomStream11RandomFloatEff_ptr = NULL;
using _imp___ZN20CUniformRandomStream7SetSeedEi_t = decltype(static_cast<void(CUniformRandomStream::*)(int)>(&CUniformRandomStream::SetSeed));
_imp___ZN20CUniformRandomStream7SetSeedEi_t _imp___ZN20CUniformRandomStream7SetSeedEi_ptr = NULL;

using _imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_t = void(CGaussianRandomStream::*)();
_imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_t _imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_ptr = NULL;
using _imp___ZN21CGaussianRandomStream11RandomFloatEff_t = decltype(static_cast<float(CGaussianRandomStream::*)(float,float)>(&CGaussianRandomStream::RandomFloat));
_imp___ZN21CGaussianRandomStream11RandomFloatEff_t _imp___ZN21CGaussianRandomStream11RandomFloatEff_ptr = NULL;
using _imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_t = decltype(static_cast<void(CGaussianRandomStream::*)(IUniformRandomStream *)>(&CGaussianRandomStream::AttachToStream));
_imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_t _imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_ptr = NULL;

[[gnu::constructor(0)]] static void load_vstdlib()
{
	if(vstdlib_loaded)
		return;

	HMODULE dll = GetModuleHandleA("vstdlib.dll");

	get_func(_imp___ZN20CUniformRandomStreamC1Ev_ptr, dll, "??0CUniformRandomStream@@QAE@XZ");
	get_func(_imp___ZN20CUniformRandomStream9RandomIntEii_ptr, dll, "?RandomInt@CUniformRandomStream@@UAEHHH@Z");
	get_func(_imp___ZN20CUniformRandomStream11RandomFloatEff_ptr, dll, "?RandomFloat@CUniformRandomStream@@UAEMMM@Z");
	get_func(_imp___ZN20CUniformRandomStream7SetSeedEi_ptr, dll, "?SetSeed@CUniformRandomStream@@UAEXH@Z");

	get_func(_imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_ptr, dll, "??0CGaussianRandomStream@@QAE@PAVIUniformRandomStream@@@Z");
	get_func(_imp___ZN21CGaussianRandomStream11RandomFloatEff_ptr, dll, "?RandomFloat@CGaussianRandomStream@@QAEMMM@Z");
	get_func(_imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_ptr, dll, "?AttachToStream@CGaussianRandomStream@@QAEXPAVIUniformRandomStream@@@Z");

	vstdlib_loaded = true;
}

DLL_EXPORT void THISCALL _imp___ZN20CUniformRandomStreamC1Ev( CUniformRandomStream *pthis )
{ load_vstdlib(); (pthis->*_imp___ZN20CUniformRandomStreamC1Ev_ptr)(); }
DLL_EXPORT int THISCALL _imp___ZN20CUniformRandomStream9RandomIntEii( CUniformRandomStream *pthis, int iMinVal, int iMaxVal )
{ load_vstdlib(); return (pthis->*_imp___ZN20CUniformRandomStream9RandomIntEii_ptr)(iMinVal, iMaxVal); }
DLL_EXPORT float THISCALL _imp___ZN20CUniformRandomStream11RandomFloatEff( CUniformRandomStream *pthis, float flMinVal, float flMaxVal )
{ load_vstdlib(); return (pthis->*_imp___ZN20CUniformRandomStream11RandomFloatEff_ptr)(flMinVal, flMaxVal); }
DLL_EXPORT void THISCALL _imp___ZN20CUniformRandomStream7SetSeedEi( CUniformRandomStream *pthis, int iSeed )
{ load_vstdlib(); (pthis->*_imp___ZN20CUniformRandomStream7SetSeedEi_ptr)(iSeed); }

DLL_EXPORT void THISCALL _imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream( CGaussianRandomStream *pthis )
{ load_vstdlib(); (pthis->*_imp___ZN21CGaussianRandomStreamC1EP20IUniformRandomStream_ptr)(); }
DLL_EXPORT float THISCALL _imp___ZN21CGaussianRandomStream11RandomFloatEff( CGaussianRandomStream *pthis, float flMinVal, float flMaxVal )
{ load_vstdlib(); return (pthis->*_imp___ZN21CGaussianRandomStream11RandomFloatEff_ptr)(flMinVal, flMaxVal); }
DLL_EXPORT void THISCALL _imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream( CGaussianRandomStream *pthis, IUniformRandomStream *pUniformStream )
{ load_vstdlib(); (pthis->*_imp___ZN21CGaussianRandomStream14AttachToStreamEP20IUniformRandomStream_ptr)(pUniformStream); }
#endif
