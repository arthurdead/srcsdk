#ifndef HACKMGR_DLLOVERRIDE_H
#define HACKMGR_DLLOVERRIDE_H

#pragma once

class CAppSystemGroup;

extern CAppSystemGroup *GetLauncherAppSystem();
extern void *Launcher_AppSystemCreateInterface(const char *pName, int *pReturnCode);

#endif
