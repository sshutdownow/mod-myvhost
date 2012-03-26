mod_myvhost.la: mod_myvhost.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_myvhost.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_myvhost.la
