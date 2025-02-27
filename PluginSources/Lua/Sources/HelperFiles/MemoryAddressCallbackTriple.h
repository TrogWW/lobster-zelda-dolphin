#pragma once

namespace Scripting
{

#ifdef __cplusplus
  extern "C" {
#endif

    typedef struct MemoryAddressCallbackTriple
    {
      unsigned int memory_start_address;
      unsigned int memory_end_address;
      int callback_ref;
    } MemoryAddressCallbackTriple;


#ifdef __cplusplus
  }
#endif

}
