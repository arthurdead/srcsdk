//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "winlite.h"
#include "sys_utils.h"
#include "engineinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _WIN32
const unsigned int SYS_NO_ERROR = NO_ERROR;
const unsigned int SYS_ERROR_INVALID_HANDLE = ERROR_INVALID_HANDLE;

const unsigned int SYS_WAIT_OBJECT_0 = WAIT_OBJECT_0;
const unsigned int SYS_WAIT_ABANDONED = WAIT_ABANDONED;

void Sys_SetLastError(unsigned long error)
{
	::SetLastError(error);
}

unsigned long Sys_GetLastError()
{
	return ::GetLastError();
}

SYSMHANDLE Sys_CreateMutex(const char *mutexName)
{
	return (SYSMHANDLE)::CreateMutex(NULL, FALSE, TEXT(mutexName));
}

void Sys_ReleaseMutex(SYSMHANDLE mutexHandle)
{
	::ReleaseMutex((HANDLE)mutexHandle);
}

unsigned int Sys_WaitForSingleObject(SYSMHANDLE mutexHandle, int milliseconds)
{
	return WaitForSingleObject((HANDLE)mutexHandle, milliseconds);
}

unsigned int Sys_RegisterWindowMessage(const char *msgName)
{
	return ::RegisterWindowMessage(msgName);
}

SYSWHANDLE Sys_FindWindow(const char *className, const char *windowName)
{
	return (SYSWHANDLE)::FindWindow(className, windowName);
}

void Sys_EnumWindows(void *callbackFunction, int lparam)
{
	::EnumWindows((WNDENUMPROC)callbackFunction, lparam);
}

void Sys_GetWindowText(SYSWHANDLE wnd, char *buffer, int bufferSize)
{
	::GetWindowText((HWND)wnd, buffer, bufferSize - 1);
}

void Sys_PostMessage(SYSWHANDLE wnd, unsigned int msg, unsigned int wParam, unsigned int lParam)
{
	::PostMessageA((HWND)wnd, msg, wParam, lParam);
}

void Sys_SetCursorPos(int x, int y)
{
	::SetCursorPos(x, y);
}

static ATOM staticWndclassAtom = 0;
static WNDCLASS staticWndclass = { NULL };

static LRESULT CALLBACK staticProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	return DefWindowProc(hwnd,msg,wparam,lparam);
}

SYSWHANDLE Sys_CreateWindowEx(const char *windowName)
{
	if (!staticWndclassAtom)
	{
		memset( &staticWndclass,0,sizeof(staticWndclass) );
		staticWndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		staticWndclass.lpfnWndProc = staticProc;
		staticWndclass.hInstance = GetModuleHandle(NULL);
		staticWndclass.hIcon = 0;
		staticWndclass.lpszClassName = windowName;
		staticWndclassAtom = ::RegisterClass( &staticWndclass );

		DWORD error = ::GetLastError();
	}

	return (SYSWHANDLE)::CreateWindow(windowName, windowName, 0, 0, 0, 0, 0, 0, 0, GetModuleHandle(NULL), 0);
}

void Sys_DestroyWindow(SYSWHANDLE wnd)
{
	::DestroyWindow((HWND)wnd);
}
#endif
