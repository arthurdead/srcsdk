//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SYS_UTILS_H
#define SYS_UTILS_H
#pragma once

#ifdef _WIN32
typedef int SYSWHANDLE;
typedef int SYSMHANDLE;

// ERROR HANDLING
extern const unsigned int SYS_NO_ERROR;
extern const unsigned int SYS_ERROR_INVALID_HANDLE;

void Sys_SetLastError(unsigned long error);
unsigned long Sys_GetLastError();

// MUTEX HANDLING
SYSMHANDLE Sys_CreateMutex(const char *mutexName);
void Sys_ReleaseMutex(SYSMHANDLE mutexHandle);

// MUTEX SYNCHRONIZATION
extern const unsigned int SYS_WAIT_OBJECT_0;
extern const unsigned int SYS_WAIT_ABANDONED;
unsigned int Sys_WaitForSingleObject(SYSMHANDLE mutexHandle, int milliseconds);

// window handling
unsigned int Sys_RegisterWindowMessage(const char *msgName);
SYSWHANDLE Sys_FindWindow(const char *className, const char *windowName);
void Sys_EnumWindows(void *callbackFunction, int lparam);
void Sys_GetWindowText(SYSWHANDLE wnd, char *buffer, int bufferSize);
void Sys_PostMessage(SYSWHANDLE wnd, unsigned int msg, unsigned int wParam, unsigned int lParam);
SYSWHANDLE Sys_CreateWindowEx(const char *windowName);
void Sys_DestroyWindow(SYSWHANDLE wnd);

// mouse
void Sys_SetCursorPos(int x, int y);
#endif

#endif // SYS_UTILS_H
