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
 */

#include "mod_myvhost_php.h"

#if !defined(__unused) 

#if defined(__GNUC__) && (__GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define __unused __attribute__((__unused__))
#else
#define __unused
#endif

#endif /* __unused */

static const char __unused cvsid[] = "$Id$";

#ifdef WITH_PHP

#pragma weak zend_alter_ini_entry = _zend_alter_ini_entry
#pragma weak zend_restore_ini_entry = _zend_restore_ini_entry

int _zend_alter_ini_entry(const char *p1 __unused, size_t p2 __unused, const char *p3 __unused, size_t p4 __unused, int p5 __unused, int p6 __unused)
{
    return -1;
}

int _zend_restore_ini_entry(const char *p1 __unused, size_t p2 __unused, int p3 __unused)
{
    return -1;
}

#endif /* WITH_PHP */
