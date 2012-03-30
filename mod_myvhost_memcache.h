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

typedef struct {
    const char *hostname;
    apr_port_t port;
    apr_uint32_t min, max, smax, ttl;
    apr_memcache_server_t *memcache_server;
} memcached_server_cfg_t;

__BEGIN_DECLS
__END_DECLS

#endif
