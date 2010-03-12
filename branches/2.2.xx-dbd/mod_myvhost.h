/*
 * Copyright (c) 2005-2010 Igor Popov <ipopovi@gmail.com>
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

#ifndef __MOD_MYVHOST_H__
#define __MOD_MYVHOST_H__

/* Maximum number of parameters which can be passed to the query */
#define MAX_PARAMS  100

/* vhost_dbd_module server configuration */
typedef struct {
    int myvhost_enabled;
    const char *label;          /* DBD prepared statement label */
    const char *sql;            /* SQL statement (or label from  DBDPrepareSQL) */
    int nparams;                /* number of params */
    int params[MAX_PARAMS];     /* array of param codes */
    int urisegs[MAX_PARAMS];    /* number of URI segments to use (0=all) */
#ifdef WITH_CACHE
    int cache_enabled;
    apr_hash_t *cache;
    apr_pool_t *pool;
#endif /* WITH_CACHE */
} myvhost_cfg_t;

/* vhost_dbd_module connection configuration */
typedef struct {
    const char *hostname;
    const char *ftp_user;
    const char *uri;
    const char *root;
    apr_hash_t *envs;
} myvhost_con_cfg_t;

/* parameter codes */
typedef enum {
    HOSTNAME, IP, PORT, URI, FTPUSER
} param_names_t;

#endif /* __MOD_MYVHOST_H__ */
