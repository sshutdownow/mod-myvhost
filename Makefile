#
# Copyright (c) 2005-2010 Igor Popov <ipopovi@gmail.com>
#
# $Id$
#

NAME = myvhost
APACHE_MODULE = mod_myvhost.so
MODULE_LA = mod_myvhost.la
SRCS = mod_myvhost.c mod_myvhost_cache.c
OBJS = $(SRCS:%.c=%.o)

ifeq (0,${MAKELEVEL})
ifeq (0,$(-shell [ apxs = 127 ]))
APXS := apxs2
else
APXS := apxs
endif
endif


UNAME := $(shell uname)

RM = rm -rf
LN = ln -sf
CP = cp -f

CFLAGS = -Wc,-W -Wc,-Wall
CFLAGS+= -DWITH_PHP
#CFLAGS+=  -DWITH_CACHE
CFLAGS+= -DDEBUG

ifeq ($(UNAME), Linux)
CFLAGS+= -DWITH_UID_GID
LDFLAGS =  -l cap
endif

default: all

all: $(APACHE_MODULE)

$(APACHE_MODULE): $(SRCS)
	$(APXS) -c $(CFLAGS) $(LDFLAGS) $(SRCS)

install: all
	$(APXS) -i -a -n $(NAME) $(MODULE_LA)

clean:
	$(RM) $(OBJS) $(APACHE_MODULE) *.slo *.lo mod_myvhost.la .libs
