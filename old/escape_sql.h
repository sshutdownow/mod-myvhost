#ifndef __ESCAPE_SQL_H__
#define __ESCAPE_SQL_H__

#include <sys/cdefs.h>
#include <sys/types.h>

size_t __P(escape_sql)(const char *from, const size_t from_length, char *to, const size_t to_length);

#endif /* __ESCAPE_SQL_H__ */
