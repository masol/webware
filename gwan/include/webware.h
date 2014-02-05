// ============================================================================
// This file is part of webware.
// Published under the Apache License, Version 2.0.
// Copyright (C) by masol.li@gmail.com.
//
// See https://github.com/masol/webhome for more information.
// ============================================================================

#ifndef _WEBWARE_H
#define _WEBWARE_H

#include "gwan.h"   // G-WAN exported functions

#ifdef __cplusplus
 extern "C" {
#endif


// ----------------------------------------------------------------------------
// // structure holding pointers for server wide persistence
// // ----------------------------------------------------------------------------
typedef struct _tag_ww_server_t
{
   kv_t *kv;   // a Key-Value store
}ww_server_t;


extern const char* getBrowserPrefix(const char *agent);
extern const char* getLangPrefix(const char *cookie,const char *accept_lang);


#ifdef __cplusplus
 }
#endif


#endif
// ============================================================================
// End of Source Code
// ============================================================================

