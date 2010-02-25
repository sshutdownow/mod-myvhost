#
# Copyright (c) 2005-2007 Igor Popov <igorpopov@newmail.ru>
#
# $Id$
#

NAME = myvhost
APACHE_MODULE = mod_myvhost.so
MODULE_LA = mod_myvhost.la
APXS = apxs
SRCS = mod_myvhost.c mod_myvhost_cache.c escape_sql.c
OBJS = mod_myvhost.o mod_myvhost_cache.o escape_sql.o
#SRCS = mod_myvhost.c mod_myvhost_cache.c mod_myvhost_php.c escape_sql.c
#OBJS = mod_myvhost.o mod_myvhost_cache.o mod_myvhost_php.o escape_sql.o

RM = rm -rf
LN = ln -sf
CP = cp -f

MYSQLCPPFLAGS = `mysql_config --include`
MYSQLLDFLAGS  = `mysql_config --libs`

CFLAGS = -Wc,-W -Wc,-Wall $(MYSQLCPPFLAGS)
CFLAGS+= -DWITH_PHP -DWITH_CACHE
#CFLAGS+= -DWITH_PHP -DWITH_UID_GID -DWITH_CACHE
CFLAGS+= -DDEBUG
LDFLAGS = -W,l$(MYSQLLDFLAGS)

default: all

all: $(APACHE_MODULE)

$(APACHE_MODULE): $(SRCS)
	$(APXS) -c $(CFLAGS) $(LDFLAGS) $(SRCS)

install: all
	$(APXS) -i -a -n $(NAME) $(MODULE_LA)

clean:
	$(RM) $(OBJS) $(APACHE_MODULE) *.slo *.lo mod_myvhost.la .libs
