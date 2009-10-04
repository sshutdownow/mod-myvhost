/*
 * Copyright (c) 2007 Igor Popov <igorpopov@newmail.ru>
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

#ifndef __MOD_MYVHOST_CACHE_H__
#define __MOD_MYVHOST_CACHE_H__

typedef struct {
    char *root;
    char *admin;
#ifdef WITH_PHP
    char *php_ini_conf;
#endif
    int hits; /* negative means vhost not found */
    apr_time_t access_time;
#ifdef WITH_UID_GID
    unsigned int uid;
    unsigned int gid;
#endif
} cache_t, *p_cache_t;

__BEGIN_DECLS
void cache_vhost_add(myvhost_cfg_t *cfg,
		    const char *hostname,
		    const char *root,
		    const char *admin,
#ifdef WITH_PHP
		    const char *php_ini_conf,
#endif
#ifdef WITH_UID_GID
		    const int uid, const int gid,
#endif
		    const int hits);
void cache_vhost_del(myvhost_cfg_t *cfg, apr_hash_t *cache, const char *host);
p_cache_t cache_vhost_find(myvhost_cfg_t *cfg, const char *hostname);
void cache_vhost_flush(myvhost_cfg_t *cfg, apr_hash_t *cache, time_t older);
__END_DECLS

#endif /* __MOD_MYVHOST_CACHE_H__ */
