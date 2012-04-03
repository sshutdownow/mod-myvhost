# New ports collection makefile for:	mod_myvhost
# Date created:				2012-03-26
# Whom:					Olli Hauer <ohauer@FreeBSD.org>
#
# $FreeBSD$
#

PORTNAME=	mod_myvhost
PORTVERSION=	0.21
CATEGORIES=	www
MASTER_SITES=	http://mod-myvhost.googlecode.com/files/

MAINTAINER=	apache@FreeBSD.org
COMMENT=	Apache module for dynamically configured mass virtual hosting with php

LICENSE=	AL2

MANUAL_PACKAGE_BUILD=	needs apr and apache with DBD options enabled

MAKE_JOBS_SAFE=	yes

USE_APACHE=	22

OPTIONS=	PHP_MODULE "Enable PHP support" On \
		DEBUG_MODULE "Enable debugging" Off

.include <bsd.port.options.mk>
.include <bsd.port.pre.mk>

.if defined(WITH_PHP_MODULE)
AP_EXTRAS+=	-DWITH_PHP
USE_PHP=	yes
WANT_PHP_MOD=	yes
.endif

.if defined(WITH_DEBUG_MODULE)
AP_EXTRAS+=	-DDEBUG
STRIP=		# do not strip debug information
.endif

AP_FAST_BUILD=	yes
AP_GENPLIST=	yes

PORTDOCS=	changelog.html documentation.html download.html \
		index.html my.css
PORTEXAMPLES=	vhosts.sql httpd.conf.add

post-extract:
	@${REINPLACE_CMD} -e "s|= apxs|= ${APXS}|" \
		-e "s|= rm|= ${RM}|" -e "s|= ln|= ${LN}|" \
		-e "s|= cp|= ${CP}|" ${WRKSRC}/Makefile

post-install:
.if !defined(NOPORTDOCS)
	@${MKDIR} ${DOCSDIR}
	@cd ${WRKSRC}/doc && ${INSTALL_DATA} ${PORTDOCS} ${DOCSDIR}
.endif
.if !defined(NOPORTEXAMPLES)
	@${MKDIR} ${EXAMPLESDIR}
	@cd ${WRKSRC}/ && ${INSTALL_DATA} ${PORTEXAMPLES} ${EXAMPLESDIR}
.endif

.include <bsd.port.post.mk>
