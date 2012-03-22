#
# $Id$
#
Summary: Apache module for dynamically configured name-based mass virtual hosting with PHP.
Name: mod_myvhost
Version: 0.21
Release: 1
License: Apache-2.0
URL: http://code.google.com/p/mod-myvhost/
Group: System Environment/Daemons
Source: http://mod-myvhost.googlecode.com/files/%{name}-%{version}.tar.gz
#Source1: 
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: httpd apr apr-util httpd-mmn = %([ -a %{_includedir}/httpd/.mmn ] && cat %{_includedir}/httpd/.mmn || echo missing)
BuildRequires: httpd-devel apr-devel apr-util-devel


%description
mod_myvhost is Apache module for dynamically configured name based mass virtual
hosting with php, virtual host's configurations and php settings are stored in
MySQL database.
No need to have every vhost in apache's configuration file, no need to restart
apache after configuration changed (vhost added|deleted, vhost rootdir changed,
vhost [un]blocked...).
After all, it is capable to change settings of PHP5 dynamically (if php is
loadable module or it is linked with apache) for any vhost. By default, it sets
open_basedir as vhost's rootdir to prevent user from stoling files from other
users and from your server, but you have ability to change almost any parameter
that exists in php.ini, for example, you can turn on safe_mode or register_globals
for particular vhost, if it has old php scripts that use global variables.


%prep
%setup -q -n %{name}-%{version}

%build
make %{_smp_mflags} -f Makefile

%install
rm -rf %{buildroot}
install -D -m644 linux-redhat/zz%{name}.conf %{buildroot}/%{_sysconfdir}/httpd/conf.d/zz%{name}.conf
install -D -m755 .libs/%{name}.so %{buildroot}/%{_libdir}/httpd/modules/%{name}.so

%clean
rm -rf %{buildroot}

%files
%defattr (-,root,root)
%doc LICENSE vhosts.sql
%doc doc/changelog.html doc/documentation.html doc/download.html doc/index.html
%attr(0755,root,root) %{_libdir}/httpd/modules/%{name}.so
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/httpd/conf.d/zz%{name}.conf

%changelog
* Wed Mar 21 2012 Igor Popov <ipopovi@gmail.com> - v0.21
- allow client set safe_mode and open_basedir
- set upload_tmp_dir and session.save_path to $DocRoot/.tmp if it exists

* Fri Mar 19 2010 Igor Popov <ipopovi@gmail.com> - v0.20b
- set upload_tmp_dir to $DocRoot/.tmp if exists
- DBD version seems to work, but missed some functionality.
- DBD verion is heavily based on mod_vhost_dbd http://code.google.com/p/dbd-modules
- DBD at least version 1.3.x of APR-Utils libraries is required.

* Thu Feb 18 2010 Igor Popov <ipopovi@gmail.com> - v0.16
- vhosts that are defined in apache config and created on fly works together

* Tue Feb 16 2010 Igor Popov <ipopovi@gmail.com> - v0.15-2
- beta version of the mod_myvhost for Apache 2.xx branch
- RPM spec for Apache 2.0.xx branch
- workaround for apr_hash_clear

* Tue Dec 05 2006 Igor Popov <ipopovi@gmail.com> - v0.15
- Just changed from using Apache's ap_psprintf to libc's snprintf,
  it allows to use more complicated directives like %1$s and
  in turn it allows very simple solvation of problem finding vhost
  by name or alias, for example:
    SELECT `rootdir` FROM `db` WHERE (`vname`='%1$s' OR `valias`='%1$s');
- Also added expiremental feature - set uid and gid for suexec.

* Sat Jun 03 2006 Igor Popov <ipopovi@gmail.com>
- Added internal implementation of mysql_escape_string().
- Some minor fixes.

* Sat Mar 25 2006 Igor Popov <ipopovi@gmail.com>
- Added experimental support for internal caching for MySQL responses,
  caching is based on apr_hash from apr 1.2.2

* Sun Mar 19 2006 Igor Popov <ipopovi@gmail.com>
- The first public release
