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

__BEGIN_DECLS
apr_memcache_t* ap_memcache_client(server_rec*);
apr_hash_t* ap_memcache_serverhash(server_rec*);
__END_DECLS

#endif
