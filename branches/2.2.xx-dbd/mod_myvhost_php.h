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

#ifndef __MOD_MYVHOST_PHP_H__
#define __MOD_MYVHOST_PHP_H__

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

#endif /* __MOD_MYVHOST_PHP_H__ */
