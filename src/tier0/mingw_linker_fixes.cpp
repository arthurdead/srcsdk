#ifdef _WIN32
#include <windows.h>
#endif

#define THREADTOOLS_H
#include "tier0/platform.h"

#undef THREADTOOLS_H
#define private public
#define protected public
#include "tier0/threadtools.h"
#undef private
#undef protected

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef __MINGW32__
static bool tier0_loaded = false;

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
using _imp___ZN17CThreadSyncObjectD2Ev_t = void (CThreadSyncObject::*)();
DLL_EXPORT_ATTR _imp___ZN17CThreadSyncObjectD2Ev_t _imp___ZN17CThreadSyncObjectD2Ev = NULL;

using _imp___ZN12CThreadEventC1Eb_t = void (CThreadEvent::*)(bool);
DLL_EXPORT_ATTR _imp___ZN12CThreadEventC1Eb_t _imp___ZN12CThreadEventC1Eb = NULL;
using _imp___ZN12CThreadEvent3SetEv_t = decltype(static_cast<bool(CThreadEvent::*)()>(&CThreadEvent::Set));
DLL_EXPORT_ATTR _imp___ZN12CThreadEvent3SetEv_t _imp___ZN12CThreadEvent3SetEv = NULL;

using _imp___ZN16CThreadLocalBaseC2Ev_t = void (CThreadLocalBase::*)();
DLL_EXPORT_ATTR _imp___ZN16CThreadLocalBaseC2Ev_t _imp___ZN16CThreadLocalBaseC2Ev = NULL;
using _imp___ZN16CThreadLocalBaseD2Ev_t = void (CThreadLocalBase::*)();
DLL_EXPORT_ATTR _imp___ZN16CThreadLocalBaseD2Ev_t _imp___ZN16CThreadLocalBaseD2Ev = NULL;
using _imp___ZNK16CThreadLocalBase3GetEv_t = decltype(static_cast<void *(CThreadLocalBase::*)() const>(&CThreadLocalBase::Get));
DLL_EXPORT_ATTR _imp___ZNK16CThreadLocalBase3GetEv_t _imp___ZNK16CThreadLocalBase3GetEv = NULL;
using _imp___ZN16CThreadLocalBase3SetEPv_t = decltype(static_cast<void(CThreadLocalBase::*)(void *)>(&CThreadLocalBase::Set));
DLL_EXPORT_ATTR _imp___ZN16CThreadLocalBase3SetEPv_t _imp___ZN16CThreadLocalBase3SetEPv = NULL;

using _imp___ZN7CThreadD2Ev_t = void (CThread::*)();
DLL_EXPORT_ATTR _imp___ZN7CThreadD2Ev_t _imp___ZN7CThreadD2Ev = NULL;
using _imp___ZN7CThread7IsAliveEv_t = decltype(static_cast<bool(CThread::*)()>(&CThread::IsAlive));
DLL_EXPORT_ATTR _imp___ZN7CThread7IsAliveEv_t _imp___ZN7CThread7IsAliveEv = NULL;
using _imp___ZN7CThread7SetNameEPKc_t = decltype(static_cast<void(CThread::*)(const char *)>(&CThread::SetName));
DLL_EXPORT_ATTR _imp___ZN7CThread7SetNameEPKc_t _imp___ZN7CThread7SetNameEPKc = NULL;
using _imp___ZN7CThread5StartEj_t = decltype(static_cast<bool(CThread::*)(unsigned)>(&CThread::Start));
DLL_EXPORT_ATTR _imp___ZN7CThread5StartEj_t _imp___ZN7CThread5StartEj = NULL;
using _imp___ZN7CThread13GetThreadProcEv_t = decltype(static_cast<CThread::ThreadProc_t(CThread::*)()>(&CThread::GetThreadProc));
DLL_EXPORT_ATTR _imp___ZN7CThread13GetThreadProcEv_t _imp___ZN7CThread13GetThreadProcEv = NULL;
using _imp___ZN7CThread15IsThreadRunningEv_t = decltype(static_cast<bool(CThread::*)()>(&CThread::IsThreadRunning));
DLL_EXPORT_ATTR _imp___ZN7CThread15IsThreadRunningEv_t _imp___ZN7CThread15IsThreadRunningEv = NULL;
using _imp___ZN7CThread4InitEv_t = decltype(static_cast<bool(CThread::*)()>(&CThread::Init));
DLL_EXPORT_ATTR _imp___ZN7CThread4InitEv_t _imp___ZN7CThread4InitEv = NULL;
using _imp___ZN7CThread6OnExitEv_t = decltype(static_cast<void(CThread::*)()>(&CThread::OnExit));
DLL_EXPORT_ATTR _imp___ZN7CThread6OnExitEv_t _imp___ZN7CThread6OnExitEv = NULL;

DLL_EXPORT_ATTR void *_imp___ZTV13CWorkerThread = NULL;
using _imp___ZN13CWorkerThreadC2Ev_t = void (CWorkerThread::*)();
DLL_EXPORT_ATTR _imp___ZN13CWorkerThreadC2Ev_t _imp___ZN13CWorkerThreadC2Ev = NULL;
using _imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_t = decltype(static_cast<int(CWorkerThread::*)(unsigned, unsigned, bool, CFunctor *)>(&CWorkerThread::CallWorker));
DLL_EXPORT_ATTR _imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_t _imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor = NULL;
using _imp___ZN13CWorkerThread5ReplyEj_t = decltype(static_cast<void(CWorkerThread::*)(unsigned)>(&CWorkerThread::Reply));
DLL_EXPORT_ATTR _imp___ZN13CWorkerThread5ReplyEj_t _imp___ZN13CWorkerThread5ReplyEj = NULL;
using _imp___ZN13CWorkerThread11WaitForCallEPj_t = decltype(static_cast<bool(CWorkerThread::*)(unsigned *pResult)>(&CWorkerThread::WaitForCall));
DLL_EXPORT_ATTR _imp___ZN13CWorkerThread11WaitForCallEPj_t _imp___ZN13CWorkerThread11WaitForCallEPj = NULL;

using _imp___ZN12CThreadMutexC1Ev_t = void (CThreadMutex::*)();
DLL_EXPORT_ATTR _imp___ZN12CThreadMutexC1Ev_t _imp___ZN12CThreadMutexC1Ev = NULL;
using _imp___ZN12CThreadMutexD1Ev_t = void (CThreadMutex::*)();
DLL_EXPORT_ATTR _imp___ZN12CThreadMutexD1Ev_t _imp___ZN12CThreadMutexD1Ev = NULL;

using _imp___ZNV16CThreadFastMutex4LockEjj_t = decltype(static_cast<void(CThreadFastMutex::*)(uint32,unsigned) volatile>(&CThreadFastMutex::Lock));
DLL_EXPORT_ATTR _imp___ZNV16CThreadFastMutex4LockEjj_t _imp___ZNV16CThreadFastMutex4LockEjj = NULL;

using _imp___ZN17CThreadSpinRWLock10UnlockReadEv_t = decltype(static_cast<void(CThreadSpinRWLock::*)()>(&CThreadSpinRWLock::UnlockRead));
DLL_EXPORT_ATTR _imp___ZN17CThreadSpinRWLock10UnlockReadEv_t _imp___ZN17CThreadSpinRWLock10UnlockReadEv = NULL;
using _imp___ZN17CThreadSpinRWLock11LockForReadEv_t = decltype(static_cast<void(CThreadSpinRWLock::*)()>(&CThreadSpinRWLock::LockForRead));
DLL_EXPORT_ATTR _imp___ZN17CThreadSpinRWLock11LockForReadEv_t _imp___ZN17CThreadSpinRWLock11LockForReadEv = NULL;
using _imp___ZN17CThreadSpinRWLock11UnlockWriteEv_t = decltype(static_cast<void(CThreadSpinRWLock::*)()>(&CThreadSpinRWLock::UnlockWrite));
DLL_EXPORT_ATTR _imp___ZN17CThreadSpinRWLock11UnlockWriteEv_t _imp___ZN17CThreadSpinRWLock11UnlockWriteEv = NULL;
using _imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_t = decltype(static_cast<void(CThreadSpinRWLock::*)(const uint32)>(&CThreadSpinRWLock::SpinLockForWrite));
DLL_EXPORT_ATTR _imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_t _imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj = NULL;

using _imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_t = decltype(static_cast<int *(CVProfile::*)(const tchar *, CounterGroup_t)>(&CVProfile::FindOrCreateCounter));
DLL_EXPORT_ATTR _imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_t _imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t = NULL;
using _imp___ZN9CVProfile12OutputReportEiPKci_t = decltype(static_cast<void(CVProfile::*)(int type, const tchar *, int )>(&CVProfile::OutputReport));
DLL_EXPORT_ATTR _imp___ZN9CVProfile12OutputReportEiPKci_t _imp___ZN9CVProfile12OutputReportEiPKci = NULL;
using _imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_t = decltype(static_cast<int(CVProfile::*)(const tchar *)>(&CVProfile::BudgetGroupNameToBudgetGroupID));
DLL_EXPORT_ATTR _imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_t _imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc = NULL;

using _imp___ZN10CVProfNode9ExitScopeEv_t = decltype(static_cast<bool(CVProfNode::*)()>(&CVProfNode::ExitScope));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode9ExitScopeEv_t _imp___ZN10CVProfNode9ExitScopeEv = NULL;
using _imp___ZN10CVProfNode10EnterScopeEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::EnterScope));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode10EnterScopeEv_t _imp___ZN10CVProfNode10EnterScopeEv = NULL;
using _imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_t = decltype(static_cast<CVProfNode *(CVProfNode::*)(const char *, int, const char *, int)>(&CVProfNode::GetSubNode));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_t _imp___ZN10CVProfNode10GetSubNodeEPKciS1_i = NULL;
using _imp___ZN10CVProfNode5PauseEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::Pause));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode5PauseEv_t _imp___ZN10CVProfNode5PauseEv = NULL;
using _imp___ZN10CVProfNode6ResumeEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::Resume));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode6ResumeEv_t _imp___ZN10CVProfNode6ResumeEv = NULL;
using _imp___ZN10CVProfNode5ResetEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::Reset));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode5ResetEv_t _imp___ZN10CVProfNode5ResetEv = NULL;
using _imp___ZN10CVProfNode9MarkFrameEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::MarkFrame));
DLL_EXPORT_ATTR _imp___ZN10CVProfNode9MarkFrameEv_t _imp___ZN10CVProfNode9MarkFrameEv = NULL;

using _imp___Z6ConMsgPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(ConMsg));
DLL_EXPORT_ATTR _imp___Z6ConMsgPKcz_t _imp___Z6ConMsgPKcz = NULL;
using _imp___Z6DevMsgPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(DevMsg));
DLL_EXPORT_ATTR _imp___Z6DevMsgPKcz_t _imp___Z6DevMsgPKcz = NULL;
using _imp___Z10ConWarningPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(ConWarning));
DLL_EXPORT_ATTR _imp___Z10ConWarningPKcz_t _imp___Z10ConWarningPKcz = NULL;
using _imp___Z11ConColorMsgRK5ColorPKcz_t = decltype(static_cast<void(*)(const Color&, const tchar*, ...)>(ConColorMsg));
DLL_EXPORT_ATTR _imp___Z11ConColorMsgRK5ColorPKcz_t _imp___Z11ConColorMsgRK5ColorPKcz = NULL;
using _imp___Z10DevWarningPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(DevWarning));
DLL_EXPORT_ATTR _imp___Z10DevWarningPKcz_t _imp___Z10DevWarningPKcz = NULL;
}

[[gnu::constructor(0)]] static void load_tier0()
{
	if(tier0_loaded)
		return;

	HMODULE dll = GetModuleHandleA("tier0.dll");

	get_func(_imp___ZN17CThreadSyncObjectD2Ev, dll, "??1CThreadSyncObject@@QAE@XZ");

	get_func(_imp___ZN12CThreadEventC1Eb, dll, "??0CThreadEvent@@QAE@_N@Z");
	get_func(_imp___ZN12CThreadEvent3SetEv, dll, "?Set@CThreadEvent@@QAE_NXZ");

	get_func(_imp___ZN16CThreadLocalBaseC2Ev, dll, "??0CThreadLocalBase@@QAE@XZ");
	get_func(_imp___ZN16CThreadLocalBaseD2Ev, dll, "??1CThreadLocalBase@@QAE@XZ");
	get_func(_imp___ZNK16CThreadLocalBase3GetEv, dll, "?Get@CThreadLocalBase@@QBEPAXXZ");
	get_func(_imp___ZN16CThreadLocalBase3SetEPv, dll, "?Set@CThreadLocalBase@@QAEXPAX@Z");

	get_func(_imp___ZN7CThreadD2Ev, dll, "??1CThread@@UAE@XZ");
	get_func(_imp___ZN7CThread7IsAliveEv, dll, "?IsAlive@CThread@@QAE_NXZ");
	get_func(_imp___ZN7CThread7SetNameEPKc, dll, "?SetName@CThread@@QAEXPBD@Z");
	get_func(_imp___ZN7CThread5StartEj, dll, "?Start@CThread@@UAE_NI@Z");
	get_func(_imp___ZN7CThread13GetThreadProcEv, dll, "?GetThreadProc@CThread@@MAEP6GIPAX@ZXZ");
	get_func(_imp___ZN7CThread15IsThreadRunningEv, dll, "?IsThreadRunning@CThread@@MAE_NXZ");
	get_func(_imp___ZN7CThread4InitEv, dll, "?Init@CThread@@MAE_NXZ");
	get_func(_imp___ZN7CThread6OnExitEv, dll, "?OnExit@CThread@@MAEXXZ");

	get_func(_imp___ZTV13CWorkerThread, dll, "??_7CWorkerThread@@6B@");
	get_func(_imp___ZN13CWorkerThreadC2Ev, dll, "??0CWorkerThread@@QAE@XZ");
	get_func(_imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor, dll, "?CallWorker@CWorkerThread@@QAEHII_NPAVCFunctor@@@Z");
	get_func(_imp___ZN13CWorkerThread5ReplyEj, dll, "?Reply@CWorkerThread@@QAEXI@Z");
	get_func(_imp___ZN13CWorkerThread11WaitForCallEPj, dll, "?WaitForCall@CWorkerThread@@QAE_NPAI@Z");

	get_func(_imp___ZN12CThreadMutexC1Ev, dll, "??0CThreadMutex@@QAE@XZ");
	get_func(_imp___ZN12CThreadMutexD1Ev, dll, "??1CThreadMutex@@QAE@XZ");

	get_func(_imp___ZNV16CThreadFastMutex4LockEjj, dll, "?Lock@CThreadFastMutex@@ACEXII@Z");

	get_func(_imp___ZN17CThreadSpinRWLock10UnlockReadEv, dll, "?UnlockRead@CThreadSpinRWLock@@QAEXXZ");
	get_func(_imp___ZN17CThreadSpinRWLock11LockForReadEv, dll, "?LockForRead@CThreadSpinRWLock@@QAEXXZ");
	get_func(_imp___ZN17CThreadSpinRWLock11UnlockWriteEv, dll, "?UnlockWrite@CThreadSpinRWLock@@QAEXXZ");
	get_func(_imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj, dll, "?SpinLockForWrite@CThreadSpinRWLock@@AAEXI@Z");

	get_func(_imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t, dll, "?FindOrCreateCounter@CVProfile@@QAEPAHPBDW4CounterGroup_t@@@Z");
	get_func(_imp___ZN9CVProfile12OutputReportEiPKci, dll, "?OutputReport@CVProfile@@QAEXHPBDH@Z");
	get_func(_imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc, dll, "?BudgetGroupNameToBudgetGroupID@CVProfile@@QAEHPBD@Z");

	get_func(_imp___ZN10CVProfNode9ExitScopeEv, dll, "?ExitScope@CVProfNode@@QAE_NXZ");
	get_func(_imp___ZN10CVProfNode10EnterScopeEv, dll, "?EnterScope@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode10GetSubNodeEPKciS1_i, dll, "?GetSubNode@CVProfNode@@QAEPAV1@PBDH0H@Z");
	get_func(_imp___ZN10CVProfNode5PauseEv, dll, "?Pause@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode6ResumeEv, dll, "?Resume@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode5ResetEv, dll, "?Reset@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode9MarkFrameEv, dll, "?MarkFrame@CVProfNode@@QAEXXZ");

	get_func(_imp___Z6ConMsgPKcz, dll, "?ConMsg@@YAXPBDZZ");
	get_func(_imp___Z6DevMsgPKcz, dll, "?DevMsg@@YAXPBDZZ");
	get_func(_imp___Z10ConWarningPKcz, dll, "?ConWarning@@YAXPBDZZ");
	get_func(_imp___Z11ConColorMsgRK5ColorPKcz, dll, "?ConColorMsg@@YAXABVColor@@PBDZZ");
	get_func(_imp___Z10DevWarningPKcz, dll, "?DevWarning@@YAXPBDZZ");

	tier0_loaded = true;
}

DLL_EXPORT bool THISCALL _ZN7CThread5StartEj( CThread *pthis, unsigned nBytesStack )
{ load_tier0(); return (pthis->*_imp___ZN7CThread5StartEj)(nBytesStack); }
DLL_EXPORT CThread::ThreadProc_t THISCALL _ZN7CThread13GetThreadProcEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread13GetThreadProcEv)(); }
DLL_EXPORT bool THISCALL _ZN7CThread15IsThreadRunningEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread15IsThreadRunningEv)(); }
DLL_EXPORT bool THISCALL _ZN7CThread4InitEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread4InitEv)(); }
DLL_EXPORT void THISCALL _ZN7CThread6OnExitEv( CThread *pthis )
{ load_tier0(); (pthis->*_imp___ZN7CThread6OnExitEv)(); }
#endif
