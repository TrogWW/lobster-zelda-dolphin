#pragma once

#include "DolphinDefinedAPIs.h"

namespace Scripting
{

#ifdef __cplusplus
  extern "C" {
#endif


    DLL_Export void* CreateNewScript(int, const char*, Dolphin_Defined_ScriptContext_APIs::PRINT_CALLBACK_FUNCTION_TYPE, Dolphin_Defined_ScriptContext_APIs::SCRIPT_END_CALLBACK_FUNCTION_TYPE);

#ifdef __cplusplus
  }
#endif

}
