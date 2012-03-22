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
 */

#ifdef WITH_MEMCACHE

#include "myvhost_include.h"
#include "mod_myvhost.h"
#include "mod_myvhost_memcache.h"

static const char __unused cvsid[] = "$$";


module AP_MODULE_DECLARE_DATA memcache_module;

typedef struct memcache_config {
  apr_memcache_t *mc;
  apr_hash_t *servers;
} memcache_config;

static void *memcache_create_config(apr_pool_t *p, server_rec *s)
{
  memcache_config *config = 
    (memcache_config *) apr_pcalloc(p, sizeof(memcache_config));

  config->mc = NULL;
  config->servers = apr_hash_make(p);
  return config;
}

static void *memcache_merge_config(apr_pool_t *pool, void *basev, 
                                   void *overridesv)
{
  memcache_config *base = (memcache_config *) basev;
  memcache_config *overrides = (memcache_config *) overridesv;
  memcache_config *config = apr_pcalloc(pool, sizeof(memcache_config));

  /*
   * is there a need to have the ability to run virtual hosts that 
   * inherit default servers and add their own?

   config->servers = apr_hash_overlay(pool, overrides->servers,
                                  base->servers);
  */

  config->mc = overrides->mc;
  config->servers = overrides->servers;

  return (void *) config;
}

static int memcache_post_config(apr_pool_t *pconf, apr_pool_t *plog,
                               apr_pool_t *ptemp, server_rec *s)
{
  memcache_config *config;
  server_rec *sp;

  for(sp = s; sp; sp = sp->next) {
    apr_status_t rv;
    apr_hash_index_t *hi; 
    void *val;
    apr_memcache_server_t *ms;
    apr_uint16_t max;

    config = (memcache_config *)ap_get_module_config(sp->module_config, 
                                               &memcache_module);

    max = apr_hash_count(config->servers);
      
    rv = apr_memcache_create(pconf, max, 0, &config->mc);
  
    if(rv != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_ERR, rv, sp, 
                   "Unable to create memcache object");
      return rv;
    }
    
    for (hi = apr_hash_first(pconf, config->servers); 
         hi; 
         hi = apr_hash_next(hi)) {
      
      apr_hash_this(hi, NULL, NULL, &val);
      ms = (apr_memcache_server_t *)val;

      rv = apr_memcache_add_server(config->mc, ms);

      if(rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, sp, 
                     "Unable to add server: %s:%d",
                     ms->host, ms->port);
        return rv;
      }
      ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, sp, 
                   "added server: %s:%d to %s:%d",
                   ms->host, ms->port, sp->server_hostname, sp->port);
    }
  }

  return OK;
}

/* parse config options */
static const char *cmd_mc_server (cmd_parms *cmd, void *doof, int argc,
                                  char *const argv[])
{
  int i;
  char *w;
  apr_status_t rv;
  apr_memcache_server_t *ms;
  apr_uint32_t min =0 ;
  apr_uint32_t max = 0;
  apr_uint32_t smax = 0;
  apr_uint32_t ttl = 0;
  char *server = NULL;
  char *host = NULL;
  char *port = NULL;
  memcache_config *config = 
    (memcache_config *)ap_get_module_config(cmd->server->module_config, 
                                         &memcache_module);

  for(i = 0; i < argc; i++) {
    w = argv[i];

    if(!strncasecmp(w, "Min=", 4)) {
      min = atoi(&w[4]);
    }
    else if(!strncasecmp(w, "Max=", 4)) {
      max = atoi(&w[4]);
    }
    else if(!strncasecmp(w, "Ttl=", 4)) {
      ttl = atoi(&w[4]);
    }
    else if(!strncasecmp(w, "Smax=", 5)) {
      smax = atoi(&w[5]);
    }
    else {
      server = apr_pstrdup(cmd->pool, w); /* save this for the hash key */
      host = apr_pstrdup(cmd->pool, w);

      port = strchr(host, ':');

      if(port) {
        *(port++) = '\0';
      }
      
      if(port == NULL || host == NULL) {
        return "Server must be in the format <host>:<port>";
      }
    }
  }

  ms = apr_pcalloc(cmd->pool, sizeof(apr_memcache_server_t));

  if(ms == NULL) {
    return "Unable to allocate new memcache server";
  }

  rv = 
    apr_memcache_server_create(cmd->pool, host, atoi(port), 
                               min, smax, max, ttl, &ms);

  if(rv != APR_SUCCESS) {
    return "Unable to connect to server";
  }

  apr_hash_set(config->servers, server, APR_HASH_KEY_STRING, ms);
  return NULL;
}

/* client function to return a memcache object */
MEMCACHE_DECLARE_NONSTD(apr_memcache_t *) ap_memcache_client(server_rec *s)
{
  memcache_config *config = 
    (memcache_config *)ap_get_module_config(s->module_config,
                                            &memcache_module);
  if(config->mc != NULL) {
    return config->mc;
  }
  
  return NULL;
}

/* client function to return a hash of memcache servers keyed on host:port */
MEMCACHE_DECLARE_NONSTD(apr_hash_t *) ap_memcache_serverhash(server_rec *s)
{
  memcache_config *config = 
    (memcache_config *)ap_get_module_config(s->module_config,
                                            &memcache_module);
  if(config->servers != NULL) {
    return config->servers;
  }
  
  return NULL;
}

static const command_rec memcache_cmds[] = {
  AP_INIT_TAKE_ARGV("MemcacheServer", cmd_mc_server, NULL, RSRC_CONF,
                   "memcached host, port, and other options"),
  {NULL}
};

static void memcache_hooks(apr_pool_t *pool)
{

  APR_REGISTER_OPTIONAL_FN(ap_memcache_client);
  APR_REGISTER_OPTIONAL_FN(ap_memcache_serverhash);

  ap_hook_post_config(memcache_post_config, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA memcache_module = {
  STANDARD20_MODULE_STUFF,
  NULL,
  NULL,
  memcache_create_config,
  memcache_merge_config,
  memcache_cmds,
  memcache_hooks
};

#endif /* WITH_MEMCACHE */

