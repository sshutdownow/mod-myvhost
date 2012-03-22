/*
 * Copyright (c) 2005-2012 Igor Popov <ipopovi@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * $Id$
 *
 */

#ifndef __MYVHOST_INCLUDE_H__
#define __MYVHOST_INCLUDE_H__

#include <sys/types.h>
#include <sys/cdefs.h>

#include "apr_strings.h"
#include "apr_lib.h"
#include "apu.h"
#include "apr_dbd.h"
#include "apu_version.h"
#include "ap_config.h"
#include "httpd.h"
#include "http_request.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "mod_dbd.h"
#ifdef WITH_MEMCACHE
#include "apr_memcache.h"
#endif


#if !defined(__unused)

#if defined(__GNUC__) && (__GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define __unused __attribute__((__unused__))
#else
#define __unused
#endif

#endif /* __unused */

#endif /* __MYVHOST_INCLUDE_H__ */
