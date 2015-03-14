First of all, this module is based on [mod\_vhost\_dbd](http://code.google.com/p/dbd-modules).

## Building mod\_myvhost from ports tree ##
Since mod\_myvhost now resides in the ports tree, the latest version of **mod\_myvhost** is /usr/ports/www/mod\_myvhost

### Prerequisities: ###
The fist of all you have to rebuild apr, apr-util and apache22 to support _DBD_ and one or some SQL drivers,
because this options are not enabled by default.
I usually check options _DBD_ and _MySQL_ while `make config` for apache22.
So:
```
# You have to have root privileges to do this
cd /usr/ports/www/apache22
make deinstall
cd /usr/ports/devel/apr1
make deinstall
cd /usr/ports/www/apache22
make config # check _DBD_ and _SQL-driver_
make all install clean # building and installing apache22 and all required libs
```

### Building and installing mod\_myvhost ###
```
# You have to have root privileges to do this
cd /usr/ports/www/mod_myvhost
make config # There are some settings available at compile time: WITH_PHP and DEBUG that are self-explanatory.
make all install clean # building and installing
```

## Database: ##
Now it is time to create database that will be used to store vhost's configurations.
You can use _/usr/local/share/examples/mod\_myvhost/vhosts.sql_ file as example for **MySQL**:
```
CREATE DATABASE `hosting`;
USE `hosting`;

CREATE TABLE `vhosts` (
    `vhost` varchar(255) NOT NULL default '',
#   `valias` varchar(255) NOT NULL default '',
    `enabled` enum('yes','no') NOT NULL default 'no',
    `rootdir` varchar(255) NOT NULL default '',
    `admin` varchar(255) default '',
    `extra_php_config` text,
    UNIQUE KEY `vhostname` (`vhost`),
    KEY `enabled` (`enabled`)
) TYPE=MyISAM COMMENT='vhosts';

GRANT SELECT ON hosting.vhosts TO 'nonpriv'@'localhost' IDENTIFIED BY 'MeGaPassvv0d';

INSERT INTO `vhosts` VALUES ('w_3.vhost.net', 'yes', '/var/www/vhosts/01', 'w_3@vhost.net', 'safe_mode=off;register_globals=On;open_basedir=/var/www/vhosts/01:/var/www/vhosts/share/pear;');
INSERT INTO `vhosts` VALUES ('www.vhost.net', 'yes', '/var/www/vhosts/02', 'www@vhost.net', '');
```

Create database and table:
```
mysql -u root -p < vhosts.sql
```

## Apache: ##
Now we have to edit apache's configuration file, you may use _/usr/local/share/examples/mod\_myvhost/httpd.conf.add_ as example:

```
<IfModule mod_myvhost.c>
DBDPersist On
DBDriver mysql
DBDParams "host=localhost dbname=hosting user=nonpriv pass=MeGaPassvv0d"
DBDocRoot "SELECT `rootdir` AS DocumentRoot,`admin` AS ServerAdmin,`extra_php_config` AS php_admin FROM `vhosts` WHERE (`vhost` LIKE %s OR CONCAT_WS('.', 'www', `vhost`) LIKE %s) AND `enabled`='yes' LIMIT 1" HOSTNAME HOSTNAME

<Directory "/var/www/vhosts">
RewriteEngine on
Options Indexes SymLinksIfOwnerMatch
AllowOverride FileInfo
Order allow,deny
Allow from all
</Directory>

</IfModule>
```


  * You must have Apache [DBD driver](http://people.apache.org/~niq/dbd.html) and [mod\_dbd](http://httpd.apache.org/docs/2.3/mod/mod_dbd.html) loaded and configured.
  * To be able to change php settins mod\_myvhost.so _MUST_ be loaded after libphp5.so.

#### DBDocRoot    SQL    [PARAM ... ](.md) ####
**SQL** is an SQL statement.
**PARAM ...** arguments are inserted into the SQL statement at request time.  Use one **PARAM** for each parameter marker in your SQL statement.  In the examples, **%s** is the parameter marker.  **PARAM** may be any of these words (space separated, not case sensitive):
| **HOSTNAME**|requested hostname| _may be NULL if there is no Host header. e.g. HTTP 1.0 or FTP requests_ |
|:------------|:-----------------|:------------------------------------------------------------------------|
| **IP**|server IP address| _as a string, never NULL_ |
| **PORT**|server port number| _as string, never NULL_ |
| **URI**|The request URI| _never NULL_ |
| **URI`n`** _where_ **`n`** _is 1-9_ |Pass only the first `n` segments of the URI to the query| _never NULL_ |

  * SQL request MUST return column that is called **DocumentRoot** and optional **ServerAdmin** and **php\_admin** columns.
  * For Apache 2.2.9+, any additional columns which are returned will set an [environment variable](http://httpd.apache.org/docs/2.3/env.html) with the same name as the column name. If the column value is NULL, any existing [environment variable](http://httpd.apache.org/docs/2.3/env.html) which matches the column name will be unset.
  * If no vhost have found in DB, than request is served as usual, i.e. you can use vhosts that are defined in _httpd.conf_.
  * **DocumentRoot** is checked to be an real direcory, and **mod\_myvhost** sets php's **open\_basedir** to **DocumentRoot** and turn on **safe\_mode**.
  * If exists directory DocumentRoot/.tmp than php's **upload\_tmp\_dir** sets to it.
  * If **php\_admin** column is returned by SQL request, module parses it and sets php. **php\_admin** string should contains pairs of php variables and their values delimited by equality sign, pairs in turn are separated by semicolon, for boolean variables you should use 1 and 0. There are should be no spaces.
  * Some databases (for example: Firebird) may report errors when statements for **DBDocRoot** are prepared with [DBDPrepareSQL](http://httpd.apache.org/docs/2.3/mod/mod_dbd.html#dbdpreparesql). In this case, [DBDPrepareSQL](http://httpd.apache.org/docs/2.3/mod/mod_dbd.html#dbdpreparesql) cannot be used for virtual host statements.
  * For Apache 2.2 prior to version 2.2.9, use parameter markers appropriate to your database.  For example: Use **?** for [ODBC](http://odbc-dbd.googlecode.com/) or SQLite.  Use **%s** for MySQL, etc.
  * For Apache 2.2.9+ always use **%s** as a parameter marker in your SQL statement.  If your SQL statement contains other **%** characters (for example, in a LIKE clause), use **%%** to prevent them from being misinterpreted as parameter markers.
  * Using a URI **PARAM** increases the number of database queries required because each request requires a new query. Using only the leading portion of the URI can reduce the number of database queries required. When multiple requests are made on the same keep-alive connection which have the same URI`n` value, only one query is performed.
  * If the URI is /alpha/beta/gamma/delta/index.html:
```
URI1 is /alpha
URI2 is /alpha/beta
URI3 is /alpha/beta/gamma
...
URI5 is /alpha/beta/gamma/delta/index.html
URI6 is /alpha/beta/gamma/delta/index.html
...
URI9 is /alpha/beta/gamma/delta/index.html
URI  is /alpha/beta/gamma/delta/index.html
```
  * If [mod\_ftp](http://httpd.apache.org/modules/) is used, an additional **PARAM** name is available. **FTPUSER** can be specified to pass the logged-in FTP user name. This parameter is not available for HTTP requests.
  * Setting **[LogLevel](http://httpd.apache.org/docs/2.3/mod/core.html#loglevel) debug** will print additional information in the Apache error log which can help to diagnose SQL query problems.

## Make it all working: ##
Just restart apache:
```
apachectl stop; sleep 3; apachectl start
```
So, it should work now.
This module is tested to compile and work on FreeBSD 7.4.