/*
 * Copyright (c) 2007-2012 Igor Popov <ipopovi@gmail.com>
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


/* cache vhosts's configuration */
typedef struct {
    const char *hostname;
    const char *ftp_user;
    const char *uri;
    const char *docroot;
    const char *admin;
#ifdef WITH_PHP
    apr_hash_t *php_ini;
#endif
#ifdef WITH_UID_GID
    unsigned int uid;
    unsigned int gid;
#endif
    apr_hash_t *envs;
    int hits; /* negative means vhost not found */
    apr_time_t access_time;
} myvhost_con_cfg_t;


__BEGIN_DECLS
apr_status_t cache_vhost_add(myvhost_cfg_t *cfg, myvhost_con_cfg_t*);
apr_status_t cache_vhost_del(myvhost_cfg_t *cfg, apr_hash_t *cache, const char *host);
myvhost_con_cfg_t* cache_vhost_find(myvhost_cfg_t *cfg, const char *hostname);
apr_status_t cache_vhost_flush(myvhost_cfg_t *cfg, time_t expired __unused);
__END_DECLS

#endif /* __MOD_MYVHOST_CACHE_H__ */
