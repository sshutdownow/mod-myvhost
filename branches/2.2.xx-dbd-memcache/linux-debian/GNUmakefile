#
# Copyright (c) 2005-2012 Igor Popov <ipopovi@gmail.com>
#
# $Id: Makefile 84 2010-11-05 10:46:23Z iPopovi $
#

builddir=..
top_srcdir=/usr/share/apache2
top_builddir=/usr/share/apache2
include /usr/share/apache2/build/special.mk

#   the used tools
#APXS=apxs2
APACHECTL=apachectl

ifeq (0,${MAKELEVEL})
ifeq (0,$(-shell [ apxs = 127 ]))
APXS := apxs2
else
APXS := apxs
endif
endif


#   additional defines, includes and libraries
DEFS=-DWITH_PHP -DDEBUG
#INCLUDES=
#LIBS=

#   the default target
all: local-shared-build

#   install the shared object file into Apache 
install: install-modules-yes
	mkdir -p $(DESTDIR)/etc/apache2/mods-available
	cp -f $(builddir)/debian/mod_myvhost.conf $(DESTDIR)/etc/apache2/mods-available/zzmod_myvhost.conf
	cp -f $(builddir)/debian/mod_myvhost.load $(DESTDIR)/etc/apache2/mods-available/zzmod_myvhost.load
#	a2enmod php5
#	a2enmod dbd
#	a2enmod zmod_myvhost

#   cleanup
clean:
	-rm -f mod_myvhost.o mod_myvhost.lo mod_myvhost.slo mod_myvhost.la 

#   simple test
test: reload
	-lynx -mime_header http://localhost/myvhost

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop

