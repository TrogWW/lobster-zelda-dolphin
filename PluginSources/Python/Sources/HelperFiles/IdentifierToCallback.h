#pragma once 

namespace Scripting
{

#ifdef __cplusplus
  extern "C" {
#endif	

    typedef struct IdentifierToCallback
    {
      size_t identifier;
      void* callback;

    } IdentifierToCallback;

#ifdef __cplusplus
  }
#endif

}
