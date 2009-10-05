/*
 * Copyright (c) 2005-2006 Igor Popov <igorpopov@newmail.ru>
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
 */

static const char cvsid[] = "$Id$";

#ifdef WITH_CACHE

#include "myvhost_include.h"

p_cache_t cache_vhost_find(myvhost_cfg_t *cfg, const char *hostname)
{
    p_cache_t vhost;
    time_t cur;
    
    assert(cfg);
    if (!cfg->cache_enabled) {
	return NULL;
    }

    vhost = ap_hash_get(cfg->cache, hostname, AP_HASH_KEY_STRING);
    if (!vhost) {
	return NULL;
    }

    cur = time(NULL);

    if (vhost->hits > 0 && vhost->hits < 512 && vhost->access_time + 300 >= cur) {
	vhost->hits++;
    } else if (vhost->hits < 0 && vhost->hits > -256 && vhost->access_time + 180 >= cur) {
	vhost->hits--;
    } else {
	ap_hash_set(cfg->cache, hostname, AP_HASH_KEY_STRING, NULL);	/* delete hash entry */
	vhost = NULL;
    }
    return vhost;
}

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
		     const int hits)
{
    p_cache_t vhost;

    assert(cfg);
    if (!cfg->cache_enabled) {
	return;
    }

    vhost = ap_pcalloc(cfg->pool, sizeof(cache_t));
    vhost->access_time = time(NULL);
    vhost->root = ap_pstrdup(cfg->pool, root);
    vhost->admin = ap_pstrdup(cfg->pool, admin);
#ifdef WITH_PHP
    vhost->php_ini_conf = ap_pstrdup(cfg->pool, php_ini_conf);
#endif
    vhost->hits = hits;
#ifdef WITH_UID_GID
    vhost->uid = uid;
    vhost->gid = gid;
#endif
    ap_hash_set(cfg->cache, hostname, AP_HASH_KEY_STRING, vhost);
}

void cache_vhost_del(myvhost_cfg_t *cfg, ap_hash_t *cache, const char *host)
{
    if (!cfg->cache_enabled) {
	return;
    }
    ap_hash_set(cache, host, AP_HASH_KEY_STRING, NULL);	/* delete hash entry */
}

void cache_vhost_flush(myvhost_cfg_t *cfg, ap_hash_t *cache, time_t older)
{
    assert(cfg);
    if (!cfg->cache_enabled) {
	return;
    }
    apr_hash_clear(cache);
}

#endif /* WITH_CACHE */
