#include "escape_sql.h"

size_t
escape_sql(const char *from, const size_t from_length, char *to, const size_t to_length)
{
  int overflow = 0, escaped;
  size_t from_index = 0, to_index = 0;
     
  if (to == 0 || from == 0 || from_length == 0 || to_length == 0) {
    return 0;
  }
  
  while (from_index < from_length) {
    escaped = 0;
    
    switch (from[from_index]) {
    case 0:
      escaped = '0';
      break;
    case '\n':
      escaped = 'n';
      break;
    case '\t':
	escaped = 't';
	break;      
    case '\r':
      escaped = 'r';
      break;
    case '\b':
	escaped = 'b';
	break;      
    case '\\':
      escaped = '\\';
      break;
    case '\'':
      escaped = '\'';
      break;
    case '"':
      escaped = '"';
      break;
    }
    
    if (escaped) {
      if (to_index + 2 > to_length) {
        overflow = 1;
        break;
      }
      to[to_index++] = '\\';
      to[to_index++] = escaped;
    } else {
      if (to_index + 1 > to_length) {
        overflow = 1;
        break;
      }
      to[to_index++] = from[from_index++];
    }
  }
  to[to_index] = 0;

  return overflow ? 0 : to_index;
}
