/* 
 * Copyright (c) 2012 Igor Popov <ipopovi@gmail.com>
 * Copyright 2007 Josh Rotenberg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * $Id$
 */

/*
 * 
 * Memcached Client Extenstion module for Apache
 *
 */

#ifndef MEMCACHE_H
#define MEMCACHE_H

#if !defined(WIN32)
#define MEMCACHE_DECLARE(type)            type
#define MEMCACHE_DECLARE_NONSTD(type)     type
#define MEMCACHE_DECLARE_DATA
#elif defined(MEMCACHE_DECLARE_STATIC)
#define MEMCACHE_DECLARE(type)            type __stdcall
#define MEMCACHE_DECLARE_NONSTD(type)     type
#define MEMCACHE_DECLARE_DATA
#elif defined(MEMCACHE_DECLARE_EXPORT)
#define MEMCACHE_DECLARE(type)            __declspec(dllexport) type __stdcall
#define MEMCACHE_DECLARE_NONSTD(type)     __declspec(dllexport) type
#define MEMCACHE_DECLARE_DATA             __declspec(dllexport)
#else
#define MEMCACHE_DECLARE(type)            __declspec(dllimport) type __stdcall
#define MEMCACHE_DECLARE_NONSTD(type)     __declspec(dllimport) type
#define MEMCACHE_DECLARE_DATA             __declspec(dllimport)
#endif

#include "apr_hooks.h"
#include "apr.h"
#include "apr_lib.h"
#include "apr_optional.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "httpd.h"
#include "http_config.h"
#include "ap_config.h"
#include "http_core.h"
#include "http_main.h"
#include "http_log.h"
#include "apr_memcache.h"

MEMCACHE_DECLARE_NONSTD(apr_memcache_t*) ap_memcache_client(server_rec *s);
MEMCACHE_DECLARE_NONSTD(apr_hash_t*) ap_memcache_serverhash(server_rec *s);

APR_DECLARE_OPTIONAL_FN(apr_memcache_t*, ap_memcache_client, (server_rec *s));
APR_DECLARE_OPTIONAL_FN(apr_hash_t*, ap_memcache_serverhash, (server_rec *s));

#endif

