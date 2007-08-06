/*
 * Copyright (c) 2005-2007 Igor Popov <igorpopov@newmail.ru>
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

#define CORE_PRIVATE

#include "ap_config.h"
#include "httpd.h"
#include "http_request.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"

#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_hash.h"

#include <mysql.h>

#include "myvhost_include.h"
#include "mod_myvhost_cache.h"
#include "mod_myvhost_php.h"
#include "escape_sql.h"

#if !defined(__unused)

#if defined(__GNUC__) && (__GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define __unused __attribute__((__unused__))
#else
#define __unused
#endif

#endif /* __unused */

#define ap_block_alarms()
#define ap_unblock_alarms()

static const char __unused cvsid[] = "$Id$";


module AP_MODULE_DECLARE_DATA myvhost_module;

static int myvhost_init(apr_pool_t *p __unused, apr_pool_t *plog __unused, apr_pool_t *ptemp __unused, server_rec *s)
{
    myvhost_cfg_t *cfg = ap_get_module_config(s->module_config, &myvhost_module);
    return OK;
}

static int myvhost_setup(server_rec *s)
{
    myvhost_cfg_t *cfg = ap_get_module_config(s->module_config, &myvhost_module);

    cfg->mysql_connected = 0;

    if (!cfg->myvhost_enabled) {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, s,
                     "module is disabled, but tried to connect to MySQL server");
        return -1;
    }
    /* This should never ever ever happen */
    if (!cfg->mysql) {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, s, "MySQL handle is NULL");
        return -1;
    }
    if (!mysql_real_connect(cfg->mysql, cfg->mysql_host, cfg->mysql_user,
                            cfg->mysql_pass, cfg->mysql_dbname, cfg->mysql_inetsock, cfg->mysql_unixsock, 0)) {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, s,
                     "failed to connect to database '%s': %s", cfg->mysql_dbname, mysql_error(cfg->mysql));
        cfg->mysql_connected = 0;
        return -1;
    }
#ifdef DEBUG
    ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, s, "mod_myvhost connected to MySQL");
#endif
    cfg->mysql_connected = 1;

    return 0;
}

static void *myvhost_create_server_config(apr_pool_t *p, server_rec *s)
{
    myvhost_cfg_t *cfg = (myvhost_cfg_t *)apr_pcalloc(p, sizeof(myvhost_cfg_t));
    return (void *)cfg;
}

static void *myvhost_merge_server_config(apr_pool_t *p, void *base, void *override)
{
    myvhost_cfg_t *new_conf = (myvhost_cfg_t *)apr_pcalloc(p, sizeof(myvhost_cfg_t));
    myvhost_cfg_t *base_conf = (myvhost_cfg_t *)base;
    myvhost_cfg_t *override_conf = (myvhost_cfg_t *)override;

    new_conf->mysql = apr_pcalloc(p, sizeof(MYSQL));
    mysql_init(new_conf->mysql);

    new_conf->myvhost_enabled = (override_conf->myvhost_enabled == 1) ? 1 : 0;
    new_conf->mysql_connected = 0;
    new_conf->mysql_host = base_conf->mysql_host;
    new_conf->mysql_user = base_conf->mysql_user;
    new_conf->mysql_pass = base_conf->mysql_pass;
    new_conf->mysql_dbname = base_conf->mysql_dbname;
    new_conf->mysql_inetsock = base_conf->mysql_inetsock;
    new_conf->mysql_unixsock = base_conf->mysql_unixsock;
    new_conf->mysql_vhost_query = (override_conf->mysql_vhost_query == NULL) ?
                                  base_conf->mysql_vhost_query : override_conf->mysql_vhost_query;
    new_conf->default_host = base_conf->default_host;
    new_conf->default_root = base_conf->default_root;
#ifdef WITH_CACHE
    new_conf->cache_enabled = (override_conf->cache_enabled == 1) ? 1 : 0;
    new_conf->pool = ap_make_sub_pool(p);

    new_conf->cache = ap_hash_overlay(new_conf->pool, override_conf->cache, base_conf->cache);
#endif
    return new_conf;
}


static apr_status_t myvhost_child_exit(void *s)
{

    myvhost_cfg_t *cfg =
        ap_get_module_config( ((server_rec*)s)->module_config, &myvhost_module);

    if (cfg->mysql_connected) {
        mysql_close(cfg->mysql);
    }
#ifdef WITH_CACHE
    apr_pool_destroy(cfg->pool);
#endif
#ifdef DEBUG
    ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, s, "child exit");
#endif
    return APR_SUCCESS;
}

static void myvhost_child_init(apr_pool_t *p, server_rec *s)
{
    myvhost_cfg_t *cfg =
        ap_get_module_config(s->module_config, &myvhost_module);

#ifdef WITH_CACHE
    cfg->pool = ap_make_sub_pool(p);
    cfg->cache = ap_hash_make(cfg->pool);
#endif
    cfg->mysql = apr_pcalloc(p, sizeof(MYSQL));
    mysql_init(cfg->mysql);
    myvhost_setup(s);
    
    apr_pool_cleanup_register(p, s, myvhost_child_exit, myvhost_child_exit);
				 
#ifdef DEBUG
    ap_log_error(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, s, "child init");
#endif
}

static  apr_status_t cleanup_mysql_result(void *result)
{
    if (result) {
        mysql_free_result((MYSQL_RES *) result);
        result = 0;
    }
    return APR_SUCCESS;
}

static void default_host(myvhost_cfg_t *cfg, core_server_config *scfg, request_rec *r)
{
    /* set default values */
    scfg->ap_document_root = apr_pstrdup(r->pool, cfg->default_root);
    r->server->server_hostname = apr_pstrdup(r->pool, cfg->default_host);
//    r->server->server_uid = ap_user_id;
//    r->server->server_gid = ap_group_id;
    r->server->server_admin = apr_pstrcat(r->pool, "webmaster@", cfg->default_host, NULL);
    r->server->is_virtual = 0;

#ifdef WITH_PHP
    if (zend_restore_ini_entry("open_basedir", sizeof("open_basedir"), PHP_INI_STAGE_RUNTIME) < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, r, "zend_restore_ini_entry() failed");
    }
    if (zend_restore_ini_entry("open_basedir", sizeof("safe_mode"), PHP_INI_STAGE_RUNTIME) < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, r, "zend_restore_ini_entry() failed");
    }
#endif
}

/*
* main function
*/
static int myvhost_translate(request_rec *r)
{
    myvhost_cfg_t *cfg =
        ap_get_module_config(r->server->module_config, &myvhost_module);
    core_server_config *scfg =
        ap_get_module_config(r->server->module_config, &core_module);
    char *query = 0;
    char *safe_hostname = 0;
    int hostname_len = 0;
    MYSQL_RES *res_set = 0;
    MYSQL_ROW row;
    int num_fields_fetched = 0;
    char *rootdir = 0;
    char *admin = 0;

#ifdef WITH_PHP
    char *php_ini_conf = 0;
#endif
#ifdef WITH_CACHE
    p_cache_t vhost = 0;
#endif
#ifdef WITH_UID_GID
      unsigned int uid, gid;	
//    unsigned int uid = ap_user_id, gid = ap_group_id;
#endif

    if (!cfg->myvhost_enabled) {
        return DECLINED;
    }

    if (r->main) {
#ifdef DEBUG
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "declined: subrequest");
#endif
        return DECLINED;
    }

    if (!cfg->mysql_vhost_query) {	/* it is seemed to be redundant, but
					 * it should be there */
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, r, "declined: !mysql_vhost_query");
        return DECLINED;
    }

    if (!r->hostname) {
#ifdef DEBUG
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                      "declined: no hostname found in request");
#endif
        return DECLINED;
    }

    if (ap_ind(r->hostname, '\'') != -1 || ap_ind(r->hostname, '\\') != -1) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                      "declined: invalid character(s) in hostname '%s'", r->hostname);
        return DECLINED;
    }

    if (r->uri == 0 || r->uri[0] != '/') {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                      "declined: uri has no leading '/'");
        return DECLINED;
    }

    if (!strcasecmp(r->hostname, cfg->default_host)) {
#ifdef DEBUG
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "declined: request to default host");
#endif
        default_host(cfg, scfg, r);
        return DECLINED;
    }

#ifdef WITH_CACHE
    if ((vhost = cache_vhost_find(cfg, r->hostname)) != 0) {
        if (vhost->hits > 0) {
            rootdir = vhost->root;
            admin = vhost->admin;
#ifdef WITH_PHP
            php_ini_conf = vhost->php_ini_conf;
#endif
#ifdef WITH_UID_GID
            uid = vhost->uid;
            gid = vhost->gid;
#endif
#ifdef DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                          "cache: vhost '%s' found in positive cache", r->hostname);
#endif
            goto VHOST_FOUND;	/* I don't like goto, but sometimes it can be
				 * usefull. */
        } else if (vhost->hits < 0) {
#ifdef DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                          "declined: vhost '%s' found in negative cache", r->hostname);
#endif
            default_host(cfg, scfg, r);
            return DECLINED;
        }
    }
#endif /* WITH_CACHE */

    if ((!cfg->mysql_connected || mysql_ping(cfg->mysql)) && myvhost_setup(r->server) < 0) {
        default_host(cfg, scfg, r);
        return DECLINED;
    }

    hostname_len = strlen(r->hostname);
    safe_hostname = apr_pcalloc(r->pool, hostname_len * 2 + 1);
#if 1
    escape_sql(r->hostname, hostname_len, safe_hostname, hostname_len * 2 + 1);
#else
#ifdef HAVE_MYSQL_REAL_ESCAPE_STRING
    mysql_real_escape_string(cfg->mysql, safe_hostname, r->hostname, hostname_len);
#else
    mysql_escape_string(safe_hostname, r->hostname, hostname_len);
#endif
#endif
    query = apr_psprintf(r->pool, cfg->mysql_vhost_query, safe_hostname, NULL);

    ap_block_alarms();		/* to prevent memleaks from mysql library */

    if (mysql_real_query(cfg->mysql, query, strlen(query))) {	/* query failed */
        ap_unblock_alarms();
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, r,
                      "declined: error in mysql query '%s' %s", query, mysql_error(cfg->mysql));
        apr_table_setn(r->subprocess_env, "MYVHOST_ERR", "MYSQL_QUERY_ERROR");
        return DECLINED;
    }
    /* we have data */
    res_set = mysql_store_result(cfg->mysql);
    apr_pool_cleanup_register(r->pool, (void *)res_set, cleanup_mysql_result, &apr_pool_cleanup_null);

    ap_unblock_alarms();

    row = mysql_fetch_row(res_set);
    if (!row) {
        ap_block_alarms();
        {
	    apr_pool_cleanup_run(r->pool, (void *)res_set, &cleanup_mysql_result);
#ifdef WITH_CACHE
            cache_vhost_add(cfg, r->hostname, 0, 0,
#ifdef WITH_PHP
                            0,
#endif
#ifdef WITH_UID_GID
                            -1, -1,
#endif
                            -1);
#endif /* WITH_CACHE */
        }
        ap_unblock_alarms();
#if defined(DEBUG) && defined(WITH_CACHE)
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                      "cache: vhost '%s' added to negative cache", r->hostname);
#endif
        apr_table_setn(r->subprocess_env, "MYVHOST_ERR", "VHOST_NOT_FOUND");
        default_host(cfg, scfg, r);
#ifdef DEBUG
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                      "declined: hostname '%s' not found", r->hostname);
#endif
        return DECLINED;
    }

    if ((num_fields_fetched = mysql_num_fields(res_set)) > 0) {
         apr_finfo_t finfo;

#ifdef  WITH_PHP
#define PHP_INDEX (2 + 1)
#else
#define	PHP_INDEX 2
#endif

#ifdef WITH_UID_GID
#define	UID_GID_INDEX (PHP_INDEX + 2)
#else
#define	UID_GID_INDEX (PHP_INDEX)
#endif

        switch (num_fields_fetched) {
        default:
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, r,
                          "there are too many fields (%d) in mysql response",
                          num_fields_fetched);
#ifdef WITH_UID_GID
        case UID_GID_INDEX:
            if (row[UID_GID_INDEX-1]) {
                gid = strtol(row[UID_GID_INDEX-1], 0, 10);
                if (!gid) {
//                    gid = ap_group_id;
                    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                                  "trying to set gid to zero");
                }
            }

        case UID_GID_INDEX-1:
            if (row[UID_GID_INDEX-2]) {
                uid = strtol(row[UID_GID_INDEX-2], 0, 10);
                if (!uid) {
//                    uid = ap_group_id;
                    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                                  "trying to set uid to root");
                }
            }
#endif /* WITH_UID_GID */
#ifdef WITH_PHP
        case PHP_INDEX:
            if (row[PHP_INDEX-1]) {
                php_ini_conf = apr_pstrdup(r->pool, row[PHP_INDEX-1]);
            }
#endif /* WITH_PHP */
        case 2:
            if (row[1]) {
                admin = apr_pstrdup(r->pool, row[1]);
            } else {
                admin = apr_pstrcat(r->pool, "webmaster@", r->hostname, NULL);
            }

        case 1:
            if (row[0]) {
                rootdir = apr_pstrdup(r->pool, row[0]);
            }
        }

        ap_block_alarms(); /* to avoid memleaks */
        {
	    apr_pool_cleanup_run(r->pool, (void *)res_set, &cleanup_mysql_result);
	}
        ap_unblock_alarms();

        if (!rootdir) {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, r,
                          "declined: no rootdir for vhost '%s'", r->hostname);
            return DECLINED;
        }

VHOST_FOUND:
        if (!ap_is_directory(r->pool, rootdir)) {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                          "declined: rootdir '%s' is not dir at all", rootdir);
            apr_table_setn(r->subprocess_env, "MYVHOST_ERR", "WRONG_ROOTDIR");
            return DECLINED;
        }

        scfg->ap_document_root = rootdir;
        r->server->server_hostname = apr_pstrdup(r->pool, r->hostname);
        r->parsed_uri.hostname = r->server->server_hostname;
        r->parsed_uri.hostinfo = r->server->server_hostname;
        r->parsed_uri.path = apr_pstrcat(r->pool, rootdir, r->parsed_uri.path, NULL);
        r->server->is_virtual = 1;
#ifdef WITH_UID_GID
//        r->server->server_gid = uid;
//        r->server->server_uid = gid;
#else
//        r->server->server_gid = ap_group_id;
//        r->server->server_uid = ap_user_id;
#endif
        r->server->server_admin = admin;
        apr_table_setn(r->subprocess_env, "SERVER_ROOT", rootdir);

#ifdef WITH_CACHE
        if (!vhost) { /* not found in cache */
            ap_block_alarms();	/* to not break cache */
            {
                cache_vhost_add(cfg, r->hostname, rootdir, admin,
#ifdef WITH_PHP
                                php_ini_conf,
#endif
#ifdef WITH_UID_GID
                                uid, gid,
#endif
                                1);
            }
            ap_unblock_alarms();
#ifdef DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                          "cache: vhost '%s' added to positive cache", r->hostname);
#endif /* DEBUG */
        }
#endif /* WITH_CACHE */

        r->filename = apr_pstrcat(r->pool, rootdir, "/", r->uri, NULL);
        ap_no2slash(r->filename);

        if (r->filename &&
	    apr_stat(&finfo, r->filename, APR_FINFO_MIN, r->pool) == APR_SUCCESS)
	{
            r->finfo = finfo;
        } else {
#ifdef DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                          "declined: file '%s' doesn't exist", r->filename);
#endif /* DEBUG */
            return DECLINED;
        }

#ifdef WITH_PHP
        apr_table_setn(r->subprocess_env, "PHP_DOCUMENT_ROOT", rootdir);
        if (zend_alter_ini_entry("open_basedir", sizeof("open_basedir"), rootdir, strlen(rootdir), PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME) < 0) {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r, "zend_alter_ini_entry() set 'open_basedir' failed");
        }

        if (zend_alter_ini_entry("safe_mode", sizeof("safe_mode"), "1", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME) < 0) {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r, "zend_alter_ini_entry() set 'safe_mode' failed");
        }

        if (php_ini_conf) {	/* there is an extra php config */
            char *linend, *value;

#ifdef DEBUG
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "php config is '%s'", php_ini_conf);
#endif
            /* Of course strtok_r is better, but it is not standard */
            while ((linend = strchr(php_ini_conf, ';')) != NULL) {
                *linend++ = '\0';
                value = strchr(php_ini_conf, '=');
                if ((value = strchr(php_ini_conf, '=')) != NULL) {
                    *value++ = '\0';
#ifdef DEBUG
                    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "setting '%s' to '%s'", php_ini_conf, value);
#endif
                    if (zend_alter_ini_entry(php_ini_conf, strlen(php_ini_conf) + 1, value, strlen(value), PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME) < 0) {
                        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, r, "zend_alter_ini_entry() failed");
                    }
                }
                if (linend) {
                    php_ini_conf = linend;
                }
            }

            if (php_ini_conf && *php_ini_conf && (value = strchr(php_ini_conf, '=')) != NULL) {
                *value++ = '\0';
#ifdef DEBUG
                ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "setting php param '%s' to value '%s'", php_ini_conf, value);
#endif
                if (zend_alter_ini_entry(php_ini_conf, strlen(php_ini_conf) + 1, value, strlen(value), PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME) < 0) {
                    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_WARNING, 0, r, "zend_alter_ini_entry() failed");
                }
            }
        }
#endif /* WITH_PHP */

#ifdef DEBUG
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "OK: translate '%s%s' to '%s'", r->hostname, r->uri, r->filename);
#endif
        return OK;
    }
    /* not for us */
#ifdef DEBUG
    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r, "declined");
#endif
    return DECLINED;
}

/*
 * config stuff
 */
static const char *set_host(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config,
                         &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "mysql db host must be set";
    }
    cfg->mysql_host = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_user(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config,
                         &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "mysql db user must be set";
    }
    cfg->mysql_user = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_pass(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config,
                         &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "mysql db passwd must be set";
    }
    cfg->mysql_pass = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_dbname(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config,
                         &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "mysql db name must be set";
    }
    cfg->mysql_dbname = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_socket(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config,
                         &myvhost_module);

    cfg->mysql_unixsock = 0;
    cfg->mysql_inetsock = 0;
    if (arg && strlen(arg) > 0) {
        if (arg[0] == '/') {
            cfg->mysql_unixsock = apr_pstrdup(cmd->pool, arg);
        } else {
            cfg->mysql_inetsock = strtol(arg, 0, 10);
        }
    }
    return NULL;
}

static const char *set_myquery(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config, &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "mysql query must be set";
    }
    cfg->mysql_vhost_query = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_module_onoff(cmd_parms *cmd, void *p1 __unused, int flag)
{
    myvhost_cfg_t *cfg = (myvhost_cfg_t *)ap_get_module_config(cmd->server->module_config, &myvhost_module);

    cfg->myvhost_enabled = (flag ? 1 : 0);
    return NULL;
}

static const char *set_default_root(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config, &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "default_root must be set";
    }
    if (!ap_is_directory(cmd->pool, arg)) {
        return "default_root must be a directory";
    }
    cfg->default_root = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_default_host(cmd_parms *cmd, void *p1 __unused, const char *arg)
{
    myvhost_cfg_t *cfg = ap_get_module_config(cmd->server->module_config, &myvhost_module);

    if (!arg || !strlen(arg)) {
        return "default_host must be set";
    }
    cfg->default_host = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

#ifdef WITH_CACHE
static const char *set_cache_onoff(cmd_parms *cmd, void *p1 __unused, int flag)
{
    myvhost_cfg_t *cfg = (myvhost_cfg_t *)ap_get_module_config(cmd->server->module_config, &myvhost_module);

    cfg->cache_enabled = (flag ? 1 : 0);
    return NULL;
}
#endif

static const command_rec myvhost_cmds[] = {
    AP_INIT_FLAG("MyVhostOn", set_module_onoff, NULL, RSRC_CONF, "Turn on Apache MySQL Vuser on this server"),
    AP_INIT_TAKE1("MyVhostDefaultHost", set_default_host, NULL, RSRC_CONF, "Set default hostname"),
    AP_INIT_TAKE1("MyVhostDefaultRoot", set_default_root, NULL, RSRC_CONF, "Set default root directory"),
    AP_INIT_TAKE1("MyVhostDbHost", set_host, NULL, RSRC_CONF, "Set hostname for MySQL server"),
    AP_INIT_TAKE1("MyVhostDbName", set_dbname, NULL, RSRC_CONF, "Set database to connect"),
    AP_INIT_TAKE1("MyVhostDbUser", set_user, NULL, RSRC_CONF, "Set username for database"),
    AP_INIT_TAKE1("MyVhostDbPass", set_pass, NULL, RSRC_CONF, "Set password for database"),
    AP_INIT_TAKE1("MyVhostDbSocket", set_socket, NULL, RSRC_CONF, "Set MySQL socket to use"),
    AP_INIT_TAKE1("MyVhostQuery", set_myquery, NULL, RSRC_CONF, "The SELECT query, returns homedir and extra php config"),
#ifdef WITH_CACHE
    AP_INIT_FLAG("MyVhostCacheOn", set_cache_onoff, NULL, RSRC_CONF, "Turn on internal caching"),
#endif
    {NULL}
};


static void register_hooks(apr_pool_t * pool __unused)
{
    ap_hook_post_config(myvhost_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(myvhost_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_translate_name(myvhost_translate, NULL, NULL, APR_HOOK_MIDDLE);
}


/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA myvhost_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                       /* create per-dir    config structures */
    NULL,                       /* merge  per-dir    config structures */
    myvhost_create_server_config, /* create per-server config structures */
    myvhost_merge_server_config, /* merge  per-server config structures */
    myvhost_cmds,             /* table of config file commands       */
    register_hooks              /* register hooks                      */
};
