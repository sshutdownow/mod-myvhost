/*
 * Copyright (c) 2005-2012 Igor Popov <ipopovi@gmail.com>
 *
 * $Id$
 *
 */

CREATE DATABASE `hosting`;
USE `hosting`;

CREATE TABLE `vhosts` (
    `vhost`   varchar(255) NOT NULL default '',
    `valias`  varchar(255) default '',
    `enabled` enum('yes','no') NOT NULL default 'no',
    `rootdir` varchar(255) NOT NULL default '',
    `admin`   varchar(255) default '',
    `extra_php_config` text,
    UNIQUE KEY `vhostname` (`vhost`, `valias`),
    KEY `enabled` (`enabled`)
) TYPE=MyISAM COMMENT='vhosts';

GRANT SELECT ON `hosting`.`vhosts` TO 'nonpriv'@'localhost' IDENTIFIED BY 'MeGaPassvv0d';

INSERT INTO `vhosts` VALUES ('v01.myvhosts.net', NULL, 'yes', '/var/www/vhosts/v01', 'www@vhosts.net', 'safe_mode=off;register_globals=On;open_basedir=/var/www/vhosts/v01:/var/www/share/pear;');
INSERT INTO `vhosts` VALUES ('v02.myvhosts.net', 'v02alias.net','yes', '/var/www/vhosts/v02', 'www@myvhosts.net', '');
INSERT INTO `vhosts` VALUES ('v03.myvhosts.net', 'anotherhost.net','yes', '/var/www/vhosts/v03', 'www@myvhosts.net', '');

COMMIT;
