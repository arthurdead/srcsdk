#ifndef HACKMGR_CREATEINTERFACE_H
#define HACKMGR_CREATEINTERFACE_H

#pragma once

#include "tier1/interface.h"

extern CreateInterfaceFn GetEngineInterfaceFactory();
extern CreateInterfaceFn GetFilesystemInterfaceFactory();
extern CreateInterfaceFn GetLauncherInterfaceFactory();
extern CreateInterfaceFn GetMaterialSystemInterfaceFactory();
extern CreateInterfaceFn GetVstdlibInterfaceFactory();

#endif
