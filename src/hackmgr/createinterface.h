#ifndef HACKMGR_CREATEINTERFACE_H
#define HACKMGR_CREATEINTERFACE_H

#pragma once

#include "tier1/interface.h"

extern CreateInterfaceFn GetEngineInterfaceFactory();
extern CreateInterfaceFn GetFilesystemInterfaceFactory();
extern CreateInterfaceFn GetLauncherInterfaceFactory();
#ifndef SWDS
extern CreateInterfaceFn GetMaterialSystemInterfaceFactory();
#endif
extern CreateInterfaceFn GetVstdlibInterfaceFactory();
extern bool IsDedicatedServer();

#endif
