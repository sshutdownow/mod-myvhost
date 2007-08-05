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
 *
 * $Id
 *
 */

#ifndef __MYVHOST_INCLUDE_H__
#define __MYVHOST_INCLUDE_H__

#include <sys/types.h>
#include <sys/stat.h>

#include "ap_config.h"
#include "httpd.h"
#include "http_request.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_conf_globals.h"
#include "util_script.h"

#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_hash.h"

#include <mysql.h>
#include "escape_sql.h"

typedef struct {

    int myvhost_enabled;
    int mysql_connected;

    MYSQL *mysql;
    char *mysql_host;
    char *mysql_user;
    char *mysql_pass;
    char *mysql_dbname;
    char *mysql_vhost_query;
    char *mysql_unixsock;
    unsigned short int mysql_inetsock;

    char *default_root;
    char *default_host;

#ifdef WITH_CACHE
    int cache_enabled;
    apr_hash_t *cache;
    apr_pool_t *pool;
#endif /* WITH_CACHE */
} myvhost_cfg_t;

typedef struct {
    char *root;
    char *admin;
#ifdef WITH_PHP
    char *php_ini_conf;
#endif
    int hits; /* negative means vhost not found */
    time_t access_time;
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
void cache_vhost_del(myvhost_cfg_t *cfg, ap_hash_t *cache, const char *host);
p_cache_t cache_vhost_find(myvhost_cfg_t *cfg, const char *hostname);
void cache_vhost_flush(myvhost_cfg_t *cfg, ap_hash_t *cache, time_t older);
__END_DECLS

#ifdef WITH_PHP

#define PHP_INI_USER    (1<<0)
#define PHP_INI_PERDIR  (1<<1)
#define PHP_INI_SYSTEM  (1<<2)

#define PHP_INI_STAGE_STARTUP           (1<<0)
#define PHP_INI_STAGE_SHUTDOWN          (1<<1)
#define PHP_INI_STAGE_ACTIVATE          (1<<2)
#define PHP_INI_STAGE_DEACTIVATE        (1<<3)
#define PHP_INI_STAGE_RUNTIME           (1<<4)

__BEGIN_DECLS
int zend_alter_ini_entry(const char *, size_t, const char *, size_t, int, int);
int zend_restore_ini_entry(const char *, size_t, int);
__END_DECLS
#endif /* WITH_PHP */

#endif /* __MYVHOST_INCLUDE_H__ */
