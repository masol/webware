// ============================================================================
// This file is part of webware.
// Published under the Apache License, Version 2.0.
// Copyright (C) by masol.li@gmail.com.
//
// See https://github.com/masol/webhome for more information.
// ============================================================================

// #pragma debug     // uncomment to get symbols/line numbers in crash reports
//
#include "gwan.h"   // G-WAN exported functions

#include <stdio.h>  // printf()
#include <string.h> // strcmp()

// ----------------------------------------------------------------------------
// // structure holding pointers for persistence
// // ----------------------------------------------------------------------------
typedef struct
{
   kv_t *kv;   // a Key-Value store
}ww_server_t;

