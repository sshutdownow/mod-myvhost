/*
 * Copyright (c) 2010 Igor Popov <ipopovi@gmail.com>
 *
 * MOD_MYVHOST DBD version
 *
 *
 * MOD_VHOST_DBD  Apache 2.2 module
 *
 * Copyright 2008 Tom Donovan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define CORE_PRIVATE

#include "myvhost_include.h"
#include "mod_myvhost.h"
#include "mod_myvhost_cache.h"
#include "mod_myvhost_php.h"

#if (APU_MAJOR_VERSION < 1) || (APU_MAJOR_VERSION == 1 && APU_MINOR_VERSION < 3)
#error "At least version 1.3.x of APR-Utils libraries is required"
#endif


static const char __unused cvsid[] = "$Id$";

module AP_MODULE_DECLARE_DATA myvhost_module;

/* parameter codes */
param_names_t paramNames;

/* optional functions imported from mod_dbd */
static APR_OPTIONAL_FN_TYPE(ap_dbd_prepare) *dbd_prepare_fn = NULL;
static APR_OPTIONAL_FN_TYPE(ap_dbd_acquire) *dbd_acquire_fn = NULL;

/* Ancillary functions */

/* check if a string could be a label name */
#define MAX_LABEL_SIZE 32
/* longest env var name */
#define MAX_ENV_NAME 32

static APR_INLINE int isSimpleName(const char *s)
{
    if (strlen(s) > MAX_LABEL_SIZE)
        return 0;
    for (; *s ; s++)
        if (!apr_isalnum(*s) && (*s != '_') && (*s != '-'))
            return 0;
    return 1;
}

static APR_INLINE int php_ini_set(char* name, char* value)
{
    return zend_alter_ini_entry(name, strlen(name)+1, value, strlen(value), PHP_INI_SYSTEM, PHP_INI_STAGE_RUNTIME);
}

/*
 * docroot restore code stollen from mod_perl :)
 */

typedef struct  {
    const char **docroot;
    const char *original;
} docroot_t, *p_docroot_t;

/* docroot cleanup handler */
static apr_status_t restore_docroot(void *data)
{
    p_docroot_t di = (p_docroot_t)data;
    *di->docroot  = di->original;
    return APR_SUCCESS;
}

/* set the document root for this request - called by a translate_name hook */
static int myvhost_translate_name(request_rec *r)
{
    request_rec *mainreq = r;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;
    ap_dbd_t *dbd;
    apr_dbd_prepared_t *stmt;
    apr_dbd_prepared_t *prestmt;
    int rows = 0;
    int cols = 0;
    int rv = 0;
    int i, j;
    const char **params;
    const char *trimmedUri;
    const char *keyHostname = NULL;
    const char *keyFTPuser = NULL;
    const char *keyUri = NULL;
    const char *root = NULL;
    const char *admin = NULL;
    const char *start;
    int maxseg = 0;
    const char *hostname = NULL;
    p_docroot_t di = 0;
#ifdef WITH_PHP
    char *php_admin = NULL;
    char *tmppath = NULL;
#endif
    apr_hash_index_t *hidx;
    myvhost_cfg_t *conf =
        (myvhost_cfg_t*) ap_get_module_config(r->server->module_config,
                                              &myvhost_module);
    myvhost_con_cfg_t *conn_conf =
        (myvhost_con_cfg_t*) ap_get_module_config(r->connection->conn_config,
                &myvhost_module);
    core_server_config *scfg = ap_get_module_config(r->server->module_config, &core_module);

    if (conf->label == NULL) {
        return DECLINED;
    }
    if (r->proxyreq) {
        return HTTP_FORBIDDEN;
    }

    hostname = ap_get_server_name(r);
    if (!hostname || !strlen(hostname)) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                      "no hostname found in request");
    } else if (ap_ind(hostname, '\'') != -1 || ap_ind(hostname, '\\') != -1) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                      "http_bad_request: invalid character(s) in hostname '%s'", hostname);
        return HTTP_BAD_REQUEST;
    }

    if (r->uri == 0 || ((r->uri[0] != '/') && apr_strnatcmp(r->uri, "*") != 0)) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "http_bad_request: Invalid URI in request %s", r->the_request);
        return HTTP_BAD_REQUEST;
    }
    params = (const char **) apr_pcalloc(r->pool, conf->nparams * sizeof(char *));

    /* Use the top-level request for FTPuser, IP, and Port */
    while (mainreq->main) {
        mainreq = mainreq->main;
    }

    /* Collect the parameters.  Make sure hostname and uri cannot surprise the
     * database server with unexpected characaters (e.g. control characters)
     * We (ab)use the ap_escape_logitem function to prevent this kind of trouble.
     */
    for (i = 0; i < conf->nparams; i++) {
        switch (conf->params[i]) {

        case HOSTNAME:
            keyHostname = ap_escape_logitem(r->pool, hostname);
            params[i] = keyHostname;
            break;

        case IP:
            params[i] = mainreq->connection->local_ip;
            break;

        case PORT:
            params[i] = apr_itoa(r->pool, mainreq->connection->local_addr->port);
            break;

        case FTPUSER:
            keyFTPuser = ap_escape_logitem(r->pool, mainreq->user);
            params[i] = keyFTPuser;
            break;

        case URI:
            start = ap_escape_uri(r->pool, r->uri);
            if ( (j = conf->urisegs[i]) && start) {
                /* extract j leading URI segments */
                const char *p = start;
                while (j && *p)
                    if (*++p == '/') --j;
                params[i] = apr_pstrndup(r->pool, start, p - start);
                if (conf->urisegs[i] > maxseg) {
                    maxseg = conf->urisegs[i];
                    keyUri = params[i];
                }
            } else {
                /* use the whole URI */
                params[i] = start;
                maxseg = 10;
                keyUri = start;
            }
        }
    }

    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                  "Hostname: %s, IP: %s, Port: %s, URI: %s",
                  hostname,
                  mainreq->connection->local_ip,
                  apr_itoa(r->pool, mainreq->connection->local_addr->port),
                  ap_escape_uri(r->pool, r->uri));

    /* Create a new connection config if we don't already have one */
    if (!conn_conf) {
        conn_conf = apr_pcalloc(r->connection->pool, sizeof(myvhost_con_cfg_t));
#ifdef WITH_PHP
        conn_conf->php_ini = apr_hash_make(r->connection->pool);
#endif
        conn_conf->envs = apr_hash_make(r->connection->pool);
        ap_set_module_config(r->connection->conn_config, &myvhost_module, conn_conf);
    }

    /* Can we re-use a previous result from our conn_conf?
     * Only a change in the hostname, FTP user name, or the part of the URI that we
     * are actually using requires a new query for this connection.
     *
     * Note that if a conn_conf->root exists we are within the same connection,
     * so this request is guaranteed to be to the same IP address.
     */
    if (!conn_conf->root
            || (keyHostname && (!conn_conf->hostname || apr_strnatcmp(conn_conf->hostname, keyHostname)))
            || (keyFTPuser &&  (!conn_conf->ftp_user  || apr_strnatcmp(conn_conf->ftp_user, keyFTPuser)))
            || (keyUri &&      (!conn_conf->uri      || apr_strnatcmp(conn_conf->uri, keyUri)))
       ) {
        /* YES - we do need to execute a query */
        if ((dbd = dbd_acquire_fn(r)) == NULL) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r,
                          "Error acquiring connection to database");
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        if ((stmt = apr_hash_get(dbd->prepared, conf->label,
                                 APR_HASH_KEY_STRING)) == NULL) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r,
                          "Unable to retrieve prepared statement %s",
                          conf->label);
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        /* conf->sql might just be a label created with DBDPrepareSQL, not SQL */
        if (isSimpleName(conf->sql) && (prestmt = apr_hash_get(dbd->prepared, conf->sql, APR_HASH_KEY_STRING))) {
            stmt = prestmt;
        }

        rv = apr_dbd_pselect(dbd->driver, r->pool, dbd->handle, &res, stmt,
                             0, conf->nparams, params);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r,
                          "Unable to execute SQL statement: %s",
                          apr_dbd_error(dbd->driver, dbd->handle, rv));
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        rows = apr_dbd_num_tuples(dbd->driver, res);
        if (rows > 1) {
            ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                          "Returned multiple (%d) rows (stmt: %s)",
                          rows, conf->label );
            /* Flush the multiple rows and return an error */
            while (!apr_dbd_get_row(dbd->driver, r->pool, res, &row, -1)) {
                continue;
            }
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        if (!rows) {
            /* a vhost was not found by the SQL query */
            /* DEBUG loglevel */
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "Executed: (stmt: %s) returned %d rows, DocumentRoot unset",
                          conf->label, rows);
            return DECLINED;
        }

        cols = apr_dbd_num_cols(dbd->driver, res);
        if (!cols) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r,
                          "mod_vhost_dbd: SQL statement returned no columns");
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        if (apr_dbd_get_row(dbd->driver, r->pool, res, &row, -1) != APR_SUCCESS) {
            if (rows < 0) {
                /* Some drivers cannot report the number of rows and return rows = -1.
                 * In this case, we didn't learn there were no rows until now.
                 */
                return DECLINED;
            }
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r,
                          "Unable to fetch 1st row of %d rows (stmt %s): %s",
                          rows, conf->label, apr_dbd_error(dbd->driver, dbd->handle, rv));
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                      "Successfully executed query: (stmt: %s) returned %d row(s) %d column(s), key: [%s:%s:%s]",
                      conf->label, rows, cols, keyHostname, keyFTPuser, keyUri);

        apr_hash_clear(conn_conf->envs);
        for (j = 0; j < cols ; j++) {
            const char *name = apr_dbd_get_name(dbd->driver, res, j);
            const char *value = apr_dbd_get_entry(dbd->driver, row, j);

            if (!name || !strlen(name)) {
                continue;
            }

            if (!apr_strnatcasecmp(name, "DocumentRoot")) {
                root = value;
                if (!root || !strlen(root)) {
                    /* fetch until -1 return to make sure results set gets cleaned up */
                    while (!apr_dbd_get_row(dbd->driver, r->pool, res, &row, -1)) {
                        continue;
                    }
                    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO |APLOG_DEBUG, 0, r,
                                  "declined: query (stmt: %s) for key [%s:%s:%s] have not found DocumentRoot",
                                  conf->label, keyHostname, keyFTPuser, keyUri);
                    return DECLINED;
                }
                ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                              "Successfully executed query: (stmt: %s) returned %d row(s) %d column(s), for key: [%s:%s:%s] DocumentRoot value is: %s",
                              conf->label, rows, cols, keyHostname, keyFTPuser, keyUri, root);
            } else if (!apr_strnatcasecmp(name, "ServerAdmin")) {
                admin = value;
                if (!admin || !strlen(admin)) {
                    admin = apr_pstrcat(r->pool, "webmaster@", hostname, NULL);
                }
            } else if (!apr_strnatcasecmp(name, "php_admin")) {
                php_admin = value;
            } else { /* save any extra columns to become env variables */
                int k;
                char str[MAX_ENV_NAME];

                apr_cpystrn(str, name, MAX_ENV_NAME);
                /* no bogus chars allowed in env var names - substitute underscores */
                for (k = 0; str[k]; k++) {
                    if (!apr_isalnum(str[k]))
                        str[k] = '_';
                }
                apr_hash_set(conn_conf->envs,
                             apr_pstrdup(r->connection->pool, str), APR_HASH_KEY_STRING,
                             apr_pstrdup(r->connection->pool, value));
            }
        }

        conn_conf->hostname = keyHostname ? apr_pstrdup(r->connection->pool, keyHostname) : NULL;
        conn_conf->ftp_user = keyFTPuser ? apr_pstrdup(r->connection->pool, keyFTPuser) : NULL;
        conn_conf->uri = keyUri ? apr_pstrdup(r->connection->pool, keyUri) : NULL;
        conn_conf->root = root ? apr_pstrdup(r->connection->pool, root) : NULL;

        /* fetch until -1 return to make sure results set gets cleaned up */
        while (!apr_dbd_get_row(dbd->driver, r->pool, res, &row, -1)) {
            continue;
        }
    } else  {
        /* NO - we do not need to execute a query. Use the root we saved in conn_conf */
        root = conn_conf->root;
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_DEBUG, 0, r,
                      "Using previous connection query (stmt: %s) key: [%s:%s:%s], setting DocumentRoot to: %s",
                      conf->label, keyHostname, keyFTPuser, keyUri, root);

    }


    if (!root) {
        return DECLINED;
    }

    trimmedUri = r->uri;
    while (*trimmedUri == '/') {
        ++trimmedUri;
    }

    if (apr_filepath_merge(&r->filename, root, trimmedUri,
                           APR_FILEPATH_TRUENAME | APR_FILEPATH_SECUREROOT,
                           r->pool)) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                      "mod_vhost_dbd: Cannot map %s to file with DocRoot %s",
                      r->the_request, root);
        return HTTP_FORBIDDEN;
    }

    /* got a good doc root - set it and save the result for this conn */
    r->canonical_filename = r->filename;
    conn_conf->root = apr_pstrdup(r->connection->pool, root);

    /* set env variables - unset them if NULL or zero-length value */
    for (hidx = apr_hash_first(r->pool, conn_conf->envs); hidx ; hidx = apr_hash_next(hidx)) {
        const char *name ;
        const char *val;
        apr_hash_this(hidx, (void *)&name, NULL, (void *)&val);
        if (val && *val) {
            apr_table_set(r->subprocess_env, name, val);
        } else {
            apr_table_unset(r->subprocess_env, name);
        }
    }

    if (!ap_is_directory(r->pool, root)) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                      "declined: rootdir '%s' is not dir at all", root);
        return DECLINED;
    }

    di = apr_palloc(r->pool, sizeof *di);
    di->docroot = &scfg->ap_document_root;
    di->original = scfg->ap_document_root;
    apr_pool_cleanup_register(r->pool, di, restore_docroot, restore_docroot);
    scfg->ap_document_root = root;
    r->server->is_virtual = 1;

#ifdef WITH_PHP
    apr_table_setn(r->subprocess_env, "PHP_DOCUMENT_ROOT", root);
    if (php_ini_set("open_basedir", root) < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r, "zend_alter_ini_entry() set 'open_basedir' failed");
    }

    if (php_ini_set("safe_mode", "1") < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r, "zend_alter_ini_entry() set 'safe_mode' failed");
    }

    if (apr_filepath_merge(&tmppath, root, ".tmp", APR_FILEPATH_NATIVE, r->pool) == APR_SUCCESS &&
            ap_is_directory(r->pool, tmppath))
    {
        if (php_ini_set("upload_tmp_dir", tmppath) < 0) {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r, "zend_alter_ini_entry() set 'upload_tmp_dir' failed");
        }
    }
#endif /* WITH_PHP */

    return (rv == APR_SUCCESS) ? OK : HTTP_BAD_REQUEST;
}

/* process DBDocRoot directive */
static const char *setVhostQuery(cmd_parms *cmd, void* mconfig __unused,
                                 const char *sql, const char *paramName)
{
    static long label_num = 0;
    myvhost_cfg_t *conf =
        (myvhost_cfg_t *) ap_get_module_config(cmd->server->module_config,
                                               &myvhost_module);
    if (!dbd_prepare_fn || !dbd_acquire_fn)
        return "mod_dbd must be enabled to use mod_vhost_dbd";

    if (conf->nparams >= MAX_PARAMS)
        return "mod_vhost_dbd: Too many parameters";

    if (!apr_strnatcasecmp(paramName, "hostname")) {
        conf->params[conf->nparams] = HOSTNAME;
    } else if (!apr_strnatcasecmp(paramName, "ip")) {
        conf->params[conf->nparams] = IP;
    } else if (!apr_strnatcasecmp(paramName, "port")) {
        conf->params[conf->nparams] = PORT;
        /* FTPUSER is only available for mod_ftp - currently un-doc'd */
    } else if (!apr_strnatcasecmp(paramName, "ftpuser")
               && ap_find_linked_module("mod_ftp.c")) {
        conf->params[conf->nparams] = FTPUSER;
    } else {
        char *uricmd = apr_pstrndup(cmd->pool, paramName, 3);
        if (!apr_strnatcasecmp(uricmd, "uri") &&
                (paramName[3]=='\0' ||
                 (apr_isdigit(paramName[3]) && paramName[4]=='\0')
                )
           )
        {
            conf->params[conf->nparams] = URI;
            conf->urisegs[conf->nparams] = atoi(paramName+3);
        } else {
            return apr_pstrcat(cmd->pool,
                               "mod_vhost_dbd: invalid parameter name: ",
                               paramName, NULL);
        }
    }
    ++conf->nparams;

    if (!conf->label) {
        conf->label = apr_pstrcat(cmd->pool, "vhost_dbd_",
                                  apr_ltoa(cmd->pool, ++label_num), NULL);
        dbd_prepare_fn(cmd->server, sql, conf->label);
        conf->sql = sql;

        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, cmd->server,
                     "mod_vhost_dbd: Prepared query (stmt: %s) from: %s",
                     conf->label, sql);
    }
    return NULL;
}

static void *merge_config_server(apr_pool_t *p __unused, void *parentconf,
                                 void *newconf)
{
    myvhost_cfg_t *base = (myvhost_cfg_t *) parentconf;
    myvhost_cfg_t *add = (myvhost_cfg_t *) newconf;

    return (add->label) ? add : base;
}

static void *config_server(apr_pool_t *p, server_rec *s __unused)
{
    myvhost_cfg_t *conf = apr_pcalloc(p, sizeof(myvhost_cfg_t));
    if (!dbd_prepare_fn) {
        dbd_prepare_fn =
            APR_RETRIEVE_OPTIONAL_FN(ap_dbd_prepare);
        dbd_acquire_fn =
            APR_RETRIEVE_OPTIONAL_FN(ap_dbd_acquire);
    }
    return conf;
}

static void register_hooks(apr_pool_t *pool __unused)
{
    static const char * const translate_pre[] = { "mod_alias.c", NULL };

    ap_hook_translate_name(myvhost_translate_name, translate_pre, NULL, APR_HOOK_MIDDLE);
}

static const command_rec cmds[] =
{
    AP_INIT_ITERATE2("DBDocRoot", setVhostQuery, NULL, RSRC_CONF,
    "DBDocRoot  QUERY  [HOSTNAME|IP|PORT|URI[n]]..."),
    {NULL}
};

module AP_MODULE_DECLARE_DATA myvhost_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,                       /* create per-dir config */
    NULL,                       /* merge per-dir config */
    config_server,              /* server config */
    merge_config_server,        /* merge server config */
    cmds,                       /* command apr_table_t */
    register_hooks              /* register hooks */
};
