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

using _imp___ZN17CThreadSyncObjectD2Ev_t = void (CThreadSyncObject::*)();
_imp___ZN17CThreadSyncObjectD2Ev_t _imp___ZN17CThreadSyncObjectD2Ev_ptr = NULL;

using _imp___ZN12CThreadEventC1Eb_t = void (CThreadEvent::*)(bool);
_imp___ZN12CThreadEventC1Eb_t _imp___ZN12CThreadEventC1Eb_ptr = NULL;
using _imp___ZN12CThreadEvent3SetEv_t = decltype(static_cast<bool(CThreadEvent::*)()>(&CThreadEvent::Set));
_imp___ZN12CThreadEvent3SetEv_t _imp___ZN12CThreadEvent3SetEv_ptr = NULL;

using _imp___ZN16CThreadLocalBaseC2Ev_t = void (CThreadLocalBase::*)();
_imp___ZN16CThreadLocalBaseC2Ev_t _imp___ZN16CThreadLocalBaseC2Ev_ptr = NULL;
using _imp___ZN16CThreadLocalBaseD2Ev_t = void (CThreadLocalBase::*)();
_imp___ZN16CThreadLocalBaseD2Ev_t _imp___ZN16CThreadLocalBaseD2Ev_ptr = NULL;
using _imp___ZNK16CThreadLocalBase3GetEv_t = decltype(static_cast<void *(CThreadLocalBase::*)() const>(&CThreadLocalBase::Get));
_imp___ZNK16CThreadLocalBase3GetEv_t _imp___ZNK16CThreadLocalBase3GetEv_ptr = NULL;
using _imp___ZN16CThreadLocalBase3SetEPv_t = decltype(static_cast<void(CThreadLocalBase::*)(void *)>(&CThreadLocalBase::Set));
_imp___ZN16CThreadLocalBase3SetEPv_t _imp___ZN16CThreadLocalBase3SetEPv_ptr = NULL;

using _imp___ZN7CThreadD2Ev_t = void (CThread::*)();
_imp___ZN7CThreadD2Ev_t _imp___ZN7CThreadD2Ev_ptr = NULL;
using _imp___ZN7CThread7IsAliveEv_t = decltype(static_cast<bool(CThread::*)()>(&CThread::IsAlive));
_imp___ZN7CThread7IsAliveEv_t _imp___ZN7CThread7IsAliveEv_ptr = NULL;
using _imp___ZN7CThread7SetNameEPKc_t = decltype(static_cast<void(CThread::*)(const char *)>(&CThread::SetName));
_imp___ZN7CThread7SetNameEPKc_t _imp___ZN7CThread7SetNameEPKc_ptr = NULL;
using _imp___ZN7CThread5StartEj_t = decltype(static_cast<bool(CThread::*)(unsigned)>(&CThread::Start));
_imp___ZN7CThread5StartEj_t _imp___ZN7CThread5StartEj_ptr = NULL;
using _imp___ZN7CThread13GetThreadProcEv_t = decltype(static_cast<CThread::ThreadProc_t(CThread::*)()>(&CThread::GetThreadProc));
_imp___ZN7CThread13GetThreadProcEv_t _imp___ZN7CThread13GetThreadProcEv_ptr = NULL;
using _imp___ZN7CThread15IsThreadRunningEv_t = decltype(static_cast<bool(CThread::*)()>(&CThread::IsThreadRunning));
_imp___ZN7CThread15IsThreadRunningEv_t _imp___ZN7CThread15IsThreadRunningEv_ptr = NULL;
using _imp___ZN7CThread4InitEv_t = decltype(static_cast<bool(CThread::*)()>(&CThread::Init));
_imp___ZN7CThread4InitEv_t _imp___ZN7CThread4InitEv_ptr = NULL;
using _imp___ZN7CThread6OnExitEv_t = decltype(static_cast<void(CThread::*)()>(&CThread::OnExit));
_imp___ZN7CThread6OnExitEv_t _imp___ZN7CThread6OnExitEv_ptr = NULL;

extern "C" {
	DLL_EXPORT_ATTR void *_imp___ZTV13CWorkerThread = NULL;
}
using _imp___ZN13CWorkerThreadC2Ev_t = void (CWorkerThread::*)();
_imp___ZN13CWorkerThreadC2Ev_t _imp___ZN13CWorkerThreadC2Ev_ptr = NULL;
using _imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_t = decltype(static_cast<int(CWorkerThread::*)(unsigned, unsigned, bool, CFunctor *)>(&CWorkerThread::CallWorker));
_imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_t _imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_ptr = NULL;
using _imp___ZN13CWorkerThread5ReplyEj_t = decltype(static_cast<void(CWorkerThread::*)(unsigned)>(&CWorkerThread::Reply));
_imp___ZN13CWorkerThread5ReplyEj_t _imp___ZN13CWorkerThread5ReplyEj_ptr = NULL;
using _imp___ZN13CWorkerThread11WaitForCallEPj_t = decltype(static_cast<bool(CWorkerThread::*)(unsigned *pResult)>(&CWorkerThread::WaitForCall));
_imp___ZN13CWorkerThread11WaitForCallEPj_t _imp___ZN13CWorkerThread11WaitForCallEPj_ptr = NULL;

using _imp___ZN12CThreadMutexC1Ev_t = void (CThreadMutex::*)();
_imp___ZN12CThreadMutexC1Ev_t _imp___ZN12CThreadMutexC1Ev_ptr = NULL;
using _imp___ZN12CThreadMutexD1Ev_t = void (CThreadMutex::*)();
_imp___ZN12CThreadMutexD1Ev_t _imp___ZN12CThreadMutexD1Ev_ptr = NULL;

using _imp___ZNV16CThreadFastMutex4LockEjj_t = decltype(static_cast<void(CThreadFastMutex::*)(uint32,unsigned) volatile>(&CThreadFastMutex::Lock));
_imp___ZNV16CThreadFastMutex4LockEjj_t _imp___ZNV16CThreadFastMutex4LockEjj_ptr = NULL;

using _imp___ZN17CThreadSpinRWLock10UnlockReadEv_t = decltype(static_cast<void(CThreadSpinRWLock::*)()>(&CThreadSpinRWLock::UnlockRead));
_imp___ZN17CThreadSpinRWLock10UnlockReadEv_t _imp___ZN17CThreadSpinRWLock10UnlockReadEv_ptr = NULL;
using _imp___ZN17CThreadSpinRWLock11LockForReadEv_t = decltype(static_cast<void(CThreadSpinRWLock::*)()>(&CThreadSpinRWLock::LockForRead));
_imp___ZN17CThreadSpinRWLock11LockForReadEv_t _imp___ZN17CThreadSpinRWLock11LockForReadEv_ptr = NULL;
using _imp___ZN17CThreadSpinRWLock11UnlockWriteEv_t = decltype(static_cast<void(CThreadSpinRWLock::*)()>(&CThreadSpinRWLock::UnlockWrite));
_imp___ZN17CThreadSpinRWLock11UnlockWriteEv_t _imp___ZN17CThreadSpinRWLock11UnlockWriteEv_ptr = NULL;
using _imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_t = decltype(static_cast<void(CThreadSpinRWLock::*)(const uint32)>(&CThreadSpinRWLock::SpinLockForWrite));
_imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_t _imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_ptr = NULL;

using _imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_t = decltype(static_cast<int *(CVProfile::*)(const tchar *, CounterGroup_t)>(&CVProfile::FindOrCreateCounter));
_imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_t _imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_ptr = NULL;
using _imp___ZN9CVProfile12OutputReportEiPKci_t = decltype(static_cast<void(CVProfile::*)(int type, const tchar *, int )>(&CVProfile::OutputReport));
_imp___ZN9CVProfile12OutputReportEiPKci_t _imp___ZN9CVProfile12OutputReportEiPKci_ptr = NULL;
using _imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_t = decltype(static_cast<int(CVProfile::*)(const tchar *)>(&CVProfile::BudgetGroupNameToBudgetGroupID));
_imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_t _imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_ptr = NULL;

using _imp___ZN10CVProfNode9ExitScopeEv_t = decltype(static_cast<bool(CVProfNode::*)()>(&CVProfNode::ExitScope));
_imp___ZN10CVProfNode9ExitScopeEv_t _imp___ZN10CVProfNode9ExitScopeEv_ptr = NULL;
using _imp___ZN10CVProfNode10EnterScopeEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::EnterScope));
_imp___ZN10CVProfNode10EnterScopeEv_t _imp___ZN10CVProfNode10EnterScopeEv_ptr = NULL;
using _imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_t = decltype(static_cast<CVProfNode *(CVProfNode::*)(const char *, int, const char *, int)>(&CVProfNode::GetSubNode));
_imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_t _imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_ptr = NULL;
using _imp___ZN10CVProfNode5PauseEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::Pause));
_imp___ZN10CVProfNode5PauseEv_t _imp___ZN10CVProfNode5PauseEv_ptr = NULL;
using _imp___ZN10CVProfNode6ResumeEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::Resume));
_imp___ZN10CVProfNode6ResumeEv_t _imp___ZN10CVProfNode6ResumeEv_ptr = NULL;
using _imp___ZN10CVProfNode5ResetEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::Reset));
_imp___ZN10CVProfNode5ResetEv_t _imp___ZN10CVProfNode5ResetEv_ptr = NULL;
using _imp___ZN10CVProfNode9MarkFrameEv_t = decltype(static_cast<void(CVProfNode::*)()>(&CVProfNode::MarkFrame));
_imp___ZN10CVProfNode9MarkFrameEv_t _imp___ZN10CVProfNode9MarkFrameEv_ptr = NULL;

using _imp___Z6ConMsgPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(ConMsg));
_imp___Z6ConMsgPKcz_t _imp___Z6ConMsgPKcz_ptr = NULL;
using _imp___Z6DevMsgPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(DevMsg));
_imp___Z6DevMsgPKcz_t _imp___Z6DevMsgPKcz_ptr = NULL;
using _imp___Z10ConWarningPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(ConWarning));
_imp___Z10ConWarningPKcz_t _imp___Z10ConWarningPKcz_ptr = NULL;
using _imp___Z11ConColorMsgRK5ColorPKcz_t = decltype(static_cast<void(*)(const Color&, const tchar*, ...)>(ConColorMsg));
_imp___Z11ConColorMsgRK5ColorPKcz_t _imp___Z11ConColorMsgRK5ColorPKcz_ptr = NULL;
using _imp___Z10DevWarningPKcz_t = decltype(static_cast<void(*)(const tchar*, ...)>(DevWarning));
_imp___Z10DevWarningPKcz_t _imp___Z10DevWarningPKcz_ptr = NULL;

[[gnu::constructor(0)]] static void load_tier0()
{
	if(tier0_loaded)
		return;

	HMODULE dll = GetModuleHandleA("tier0.dll");

	get_func(_imp___ZN17CThreadSyncObjectD2Ev_ptr, dll, "??1CThreadSyncObject@@QAE@XZ");

	get_func(_imp___ZN12CThreadEventC1Eb_ptr, dll, "??0CThreadEvent@@QAE@_N@Z");
	get_func(_imp___ZN12CThreadEvent3SetEv_ptr, dll, "?Set@CThreadEvent@@QAE_NXZ");

	get_func(_imp___ZN16CThreadLocalBaseC2Ev_ptr, dll, "??0CThreadLocalBase@@QAE@XZ");
	get_func(_imp___ZN16CThreadLocalBaseD2Ev_ptr, dll, "??1CThreadLocalBase@@QAE@XZ");
	get_func(_imp___ZNK16CThreadLocalBase3GetEv_ptr, dll, "?Get@CThreadLocalBase@@QBEPAXXZ");
	get_func(_imp___ZN16CThreadLocalBase3SetEPv_ptr, dll, "?Set@CThreadLocalBase@@QAEXPAX@Z");

	get_func(_imp___ZN7CThreadD2Ev_ptr, dll, "??1CThread@@UAE@XZ");
	get_func(_imp___ZN7CThread7IsAliveEv_ptr, dll, "?IsAlive@CThread@@QAE_NXZ");
	get_func(_imp___ZN7CThread7SetNameEPKc_ptr, dll, "?SetName@CThread@@QAEXPBD@Z");
	get_func(_imp___ZN7CThread5StartEj_ptr, dll, "?Start@CThread@@UAE_NI@Z");
	get_func(_imp___ZN7CThread13GetThreadProcEv_ptr, dll, "?GetThreadProc@CThread@@MAEP6GIPAX@ZXZ");
	get_func(_imp___ZN7CThread15IsThreadRunningEv_ptr, dll, "?IsThreadRunning@CThread@@MAE_NXZ");
	get_func(_imp___ZN7CThread4InitEv_ptr, dll, "?Init@CThread@@MAE_NXZ");
	get_func(_imp___ZN7CThread6OnExitEv_ptr, dll, "?OnExit@CThread@@MAEXXZ");

	get_func(_imp___ZTV13CWorkerThread, dll, "??_7CWorkerThread@@6B@");
	get_func(_imp___ZN13CWorkerThreadC2Ev_ptr, dll, "??0CWorkerThread@@QAE@XZ");
	get_func(_imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_ptr, dll, "?CallWorker@CWorkerThread@@QAEHII_NPAVCFunctor@@@Z");
	get_func(_imp___ZN13CWorkerThread5ReplyEj_ptr, dll, "?Reply@CWorkerThread@@QAEXI@Z");
	get_func(_imp___ZN13CWorkerThread11WaitForCallEPj_ptr, dll, "?WaitForCall@CWorkerThread@@QAE_NPAI@Z");

	get_func(_imp___ZN12CThreadMutexC1Ev_ptr, dll, "??0CThreadMutex@@QAE@XZ");
	get_func(_imp___ZN12CThreadMutexD1Ev_ptr, dll, "??1CThreadMutex@@QAE@XZ");

	get_func(_imp___ZNV16CThreadFastMutex4LockEjj_ptr, dll, "?Lock@CThreadFastMutex@@ACEXII@Z");

	get_func(_imp___ZN17CThreadSpinRWLock10UnlockReadEv_ptr, dll, "?UnlockRead@CThreadSpinRWLock@@QAEXXZ");
	get_func(_imp___ZN17CThreadSpinRWLock11LockForReadEv_ptr, dll, "?LockForRead@CThreadSpinRWLock@@QAEXXZ");
	get_func(_imp___ZN17CThreadSpinRWLock11UnlockWriteEv_ptr, dll, "?UnlockWrite@CThreadSpinRWLock@@QAEXXZ");
	get_func(_imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_ptr, dll, "?SpinLockForWrite@CThreadSpinRWLock@@AAEXI@Z");

	get_func(_imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_ptr, dll, "?FindOrCreateCounter@CVProfile@@QAEPAHPBDW4CounterGroup_t@@@Z");
	get_func(_imp___ZN9CVProfile12OutputReportEiPKci_ptr, dll, "?OutputReport@CVProfile@@QAEXHPBDH@Z");
	get_func(_imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_ptr, dll, "?BudgetGroupNameToBudgetGroupID@CVProfile@@QAEHPBD@Z");

	get_func(_imp___ZN10CVProfNode9ExitScopeEv_ptr, dll, "?ExitScope@CVProfNode@@QAE_NXZ");
	get_func(_imp___ZN10CVProfNode10EnterScopeEv_ptr, dll, "?EnterScope@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_ptr, dll, "?GetSubNode@CVProfNode@@QAEPAV1@PBDH0H@Z");
	get_func(_imp___ZN10CVProfNode5PauseEv_ptr, dll, "?Pause@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode6ResumeEv_ptr, dll, "?Resume@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode5ResetEv_ptr, dll, "?Reset@CVProfNode@@QAEXXZ");
	get_func(_imp___ZN10CVProfNode9MarkFrameEv_ptr, dll, "?MarkFrame@CVProfNode@@QAEXXZ");

	get_func(_imp___Z6ConMsgPKcz_ptr, dll, "?ConMsg@@YAXPBDZZ");
	get_func(_imp___Z6DevMsgPKcz_ptr, dll, "?DevMsg@@YAXPBDZZ");
	get_func(_imp___Z10ConWarningPKcz_ptr, dll, "?ConWarning@@YAXPBDZZ");
	get_func(_imp___Z11ConColorMsgRK5ColorPKcz_ptr, dll, "?ConColorMsg@@YAXABVColor@@PBDZZ");
	get_func(_imp___Z10DevWarningPKcz_ptr, dll, "?DevWarning@@YAXPBDZZ");

	tier0_loaded = true;
}

DLL_EXPORT void THISCALL _imp___ZN17CThreadSyncObjectD2Ev( CThreadSyncObject *pthis )
{ load_tier0(); (pthis->*_imp___ZN17CThreadSyncObjectD2Ev_ptr)(); }

DLL_EXPORT void THISCALL _imp___ZN12CThreadEventC1Eb( CThreadEvent *pthis, bool fManualReset )
{ load_tier0(); (pthis->*_imp___ZN12CThreadEventC1Eb_ptr)(fManualReset); }
DLL_EXPORT bool THISCALL _imp___ZN12CThreadEvent3SetEv( CThreadEvent *pthis )
{ load_tier0(); return (pthis->*_imp___ZN12CThreadEvent3SetEv_ptr)(); }

DLL_EXPORT void THISCALL _imp___ZN16CThreadLocalBaseC2Ev( CThreadLocalBase *pthis )
{ load_tier0(); (pthis->*_imp___ZN16CThreadLocalBaseC2Ev_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN16CThreadLocalBaseD2Ev( CThreadLocalBase *pthis )
{ load_tier0(); (pthis->*_imp___ZN16CThreadLocalBaseD2Ev_ptr)(); }
DLL_EXPORT void * THISCALL _imp___ZNK16CThreadLocalBase3GetEv( const CThreadLocalBase *pthis )
{ load_tier0(); return (pthis->*_imp___ZNK16CThreadLocalBase3GetEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN16CThreadLocalBase3SetEPv( CThreadLocalBase *pthis, void *ptr )
{ load_tier0(); (pthis->*_imp___ZN16CThreadLocalBase3SetEPv_ptr)(ptr); }

DLL_EXPORT void THISCALL _imp___ZN7CThreadD2Ev( CThread *pthis )
{ load_tier0(); (pthis->*_imp___ZN7CThreadD2Ev_ptr)(); }
DLL_EXPORT bool THISCALL _imp___ZN7CThread7IsAliveEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread7IsAliveEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN7CThread7SetNameEPKc( CThread *pthis, const char *arg1 )
{ load_tier0(); (pthis->*_imp___ZN7CThread7SetNameEPKc_ptr)(arg1); }
DLL_EXPORT bool THISCALL _imp___ZN7CThread5StartEj( CThread *pthis, unsigned nBytesStack )
{ load_tier0(); return (pthis->*_imp___ZN7CThread5StartEj_ptr)(nBytesStack); }
DLL_EXPORT CThread::ThreadProc_t THISCALL _imp___ZN7CThread13GetThreadProcEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread13GetThreadProcEv_ptr)(); }
DLL_EXPORT bool THISCALL _imp___ZN7CThread15IsThreadRunningEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread15IsThreadRunningEv_ptr)(); }
DLL_EXPORT bool THISCALL _imp___ZN7CThread4InitEv( CThread *pthis )
{ load_tier0(); return (pthis->*_imp___ZN7CThread4InitEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN7CThread6OnExitEv( CThread *pthis )
{ load_tier0(); (pthis->*_imp___ZN7CThread6OnExitEv_ptr)(); }

DLL_EXPORT void THISCALL _imp___ZN13CWorkerThreadC2Ev( CWorkerThread *pthis )
{ load_tier0(); (pthis->*_imp___ZN13CWorkerThreadC2Ev_ptr)(); }
DLL_EXPORT int THISCALL _imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor( CWorkerThread *pthis, unsigned arg1, unsigned timeout, bool fBoostWorkerPriorityToMaster, CFunctor *pParamFunctor )
{ load_tier0(); return (pthis->*_imp___ZN13CWorkerThread10CallWorkerEjjbP8CFunctor_ptr)(arg1, timeout, fBoostWorkerPriorityToMaster, pParamFunctor); }
DLL_EXPORT void THISCALL _imp___ZN13CWorkerThread5ReplyEj( CWorkerThread *pthis, unsigned arg1 )
{ load_tier0(); (pthis->*_imp___ZN13CWorkerThread5ReplyEj_ptr)(arg1); }
DLL_EXPORT bool THISCALL _imp___ZN13CWorkerThread11WaitForCallEPj( CWorkerThread *pthis, unsigned *pResult )
{ load_tier0(); return (pthis->*_imp___ZN13CWorkerThread11WaitForCallEPj_ptr)(pResult); }

DLL_EXPORT void THISCALL _imp___ZN12CThreadMutexC1Ev( CThreadMutex *pthis )
{ load_tier0(); (pthis->*_imp___ZN12CThreadMutexC1Ev_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN12CThreadMutexD1Ev( CThreadMutex *pthis )
{ load_tier0(); (pthis->*_imp___ZN12CThreadMutexD1Ev_ptr)(); }

DLL_EXPORT void THISCALL _imp___ZNV16CThreadFastMutex4LockEjj( volatile CThreadFastMutex *pthis, const uint32 threadId, unsigned nSpinSleepTime)
{ load_tier0(); (pthis->*_imp___ZNV16CThreadFastMutex4LockEjj_ptr)(threadId, nSpinSleepTime); }

DLL_EXPORT void THISCALL _imp___ZN17CThreadSpinRWLock10UnlockReadEv( CThreadSpinRWLock *pthis )
{ load_tier0(); (pthis->*_imp___ZN17CThreadSpinRWLock10UnlockReadEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN17CThreadSpinRWLock11LockForReadEv( CThreadSpinRWLock *pthis )
{ load_tier0(); (pthis->*_imp___ZN17CThreadSpinRWLock11LockForReadEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN17CThreadSpinRWLock11UnlockWriteEv( CThreadSpinRWLock *pthis )
{ load_tier0(); (pthis->*_imp___ZN17CThreadSpinRWLock11UnlockWriteEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj( CThreadSpinRWLock *pthis, const uint32 threadId )
{ load_tier0(); (pthis->*_imp___ZN17CThreadSpinRWLock16SpinLockForWriteEj_ptr)(threadId); }

DLL_EXPORT int * THISCALL _imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t( CVProfile *pthis, const tchar *pName, CounterGroup_t eCounterGroup )
{ load_tier0(); return (pthis->*_imp___ZN9CVProfile19FindOrCreateCounterEPKc14CounterGroup_t_ptr)(pName, eCounterGroup); }
DLL_EXPORT void THISCALL _imp___ZN9CVProfile12OutputReportEiPKci( CVProfile *pthis, int type, const tchar *pszStartNode, int budgetGroupID )
{ load_tier0(); (pthis->*_imp___ZN9CVProfile12OutputReportEiPKci_ptr)(type, pszStartNode, budgetGroupID); }
DLL_EXPORT int THISCALL _imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc( CVProfile *pthis, const tchar *pBudgetGroupName )
{ load_tier0(); return (pthis->*_imp___ZN9CVProfile30BudgetGroupNameToBudgetGroupIDEPKc_ptr)(pBudgetGroupName); }

DLL_EXPORT bool THISCALL _imp___ZN10CVProfNode9ExitScopeEv( CVProfNode *pthis )
{ load_tier0(); return (pthis->*_imp___ZN10CVProfNode9ExitScopeEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN10CVProfNode10EnterScopeEv( CVProfNode *pthis )
{ load_tier0(); (pthis->*_imp___ZN10CVProfNode10EnterScopeEv_ptr)(); }
DLL_EXPORT CVProfNode * THISCALL _imp___ZN10CVProfNode10GetSubNodeEPKciS1_i( CVProfNode *pthis, const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, int budgetFlags )
{ load_tier0(); return (pthis->*_imp___ZN10CVProfNode10GetSubNodeEPKciS1_i_ptr)(pszName, detailLevel, pBudgetGroupName, budgetFlags); }
DLL_EXPORT void THISCALL _imp___ZN10CVProfNode5PauseEv( CVProfNode *pthis )
{ load_tier0(); (pthis->*_imp___ZN10CVProfNode5PauseEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN10CVProfNode6ResumeEv( CVProfNode *pthis )
{ load_tier0(); (pthis->*_imp___ZN10CVProfNode6ResumeEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN10CVProfNode5ResetEv( CVProfNode *pthis )
{ load_tier0(); (pthis->*_imp___ZN10CVProfNode5ResetEv_ptr)(); }
DLL_EXPORT void THISCALL _imp___ZN10CVProfNode9MarkFrameEv( CVProfNode *pthis )
{ load_tier0(); (pthis->*_imp___ZN10CVProfNode9MarkFrameEv_ptr)(); }

DLL_EXPORT void THISCALL _imp___Z6ConMsgPKcz( PRINTF_FORMAT_STRING const tchar* pMsg, ... )
{ load_tier0(); __builtin_apply(flatten_func_type(_imp___Z6ConMsgPKcz_ptr), __builtin_apply_args(), 32); }
DLL_EXPORT void THISCALL _imp___Z6DevMsgPKcz( PRINTF_FORMAT_STRING const tchar* pMsg, ... )
{ load_tier0(); __builtin_apply(flatten_func_type(_imp___Z6DevMsgPKcz_ptr), __builtin_apply_args(), 32); }
DLL_EXPORT void THISCALL _imp___Z10ConWarningPKcz( PRINTF_FORMAT_STRING const tchar* pMsg, ... )
{ load_tier0(); __builtin_apply(flatten_func_type(_imp___Z10ConWarningPKcz_ptr), __builtin_apply_args(), 32); }
DLL_EXPORT void THISCALL _imp___Z11ConColorMsgRK5ColorPKcz( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... )
{ load_tier0(); __builtin_apply(flatten_func_type(_imp___Z11ConColorMsgRK5ColorPKcz_ptr), __builtin_apply_args(), 32); }
DLL_EXPORT void THISCALL _imp___Z10DevWarningPKcz( PRINTF_FORMAT_STRING const tchar* pMsg, ... )
{ load_tier0(); __builtin_apply(flatten_func_type(_imp___Z10DevWarningPKcz_ptr), __builtin_apply_args(), 32); }
#endif
