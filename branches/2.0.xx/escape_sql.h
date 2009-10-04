/*
  Copyright (c) 2005-2006 Igor Popov <igorpopov@newmail.ru>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0
		
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  $Id$
  
*/

#ifndef __ESCAPE_SQL_H__
#define __ESCAPE_SQL_H__

size_t __P(escape_sql) (const char *from, const size_t from_length, char *to, const size_t to_length);

#endif /* __ESCAPE_SQL_H__ */
