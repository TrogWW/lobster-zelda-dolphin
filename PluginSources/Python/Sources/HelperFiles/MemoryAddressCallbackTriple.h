#pragma once

#include "IdentifierToCallback.h"

namespace Scripting
{

#ifdef __cplusplus
  extern "C" {
#endif

    typedef struct MemoryAddressCallbackTriple
    {
      unsigned int memory_start_address;
      unsigned int memory_end_address;
      IdentifierToCallback identifier_to_callback;
    } MemoryAddressCallbackTriple;

#ifdef __cplusplus
  }
#endif
}
