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
 * $Id$
 *
 */

#ifndef __MOD_MYVHOST_H__
#define __MOD_MYVHOST_H__

typedef struct {

    int myvhost_enabled;
    int mysql_connected;

//    MYSQL *mysql;
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

#endif /* __MOD_MYVHOST_H__ */
