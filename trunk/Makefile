#
# Copyright (c) 2005-2010 Igor Popov <ipopovi@gmail.com>
#
# $Id$
#

NAME = myvhost
APACHE_MODULE = mod_myvhost.so
MODULE_LA = mod_myvhost.la
APXS = apxs
SRCS = mod_myvhost.c mod_myvhost_cache.c
OBJS = mod_myvhost.o mod_myvhost_cache.o

RM = rm -rf
LN = ln -sf
CP = cp -f

CFLAGS = -Wc,-W -Wc,-Wall
CFLAGS+= -DWITH_PHP
#CFLAGS+= -DWITH_PHP -DWITH_UID_GID -DWITH_CACHE
CFLAGS+= -DDEBUG
LDFLAGS = 

default: all

all: $(APACHE_MODULE)

$(APACHE_MODULE): $(SRCS)
	$(APXS) -c $(CFLAGS) $(LDFLAGS) $(SRCS)

install: all
	$(APXS) -i -a -n $(NAME) $(MODULE_LA)

clean:
	$(RM) $(OBJS) $(APACHE_MODULE) *.slo *.lo mod_myvhost.la .libs
