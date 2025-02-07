/*
 * Copyright (c) 2005-2010 Igor Popov <ipopovi@gmail.com>
 *
 * $Id$
 *
 */

CREATE DATABASE IF NOT EXISTS `hosting`;
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
) COMMENT='vhosts';

CREATE USER 'nonpriv'@'localhost' IDENTIFIED BY 'MeGaPassvv0d';
GRANT SELECT ON `hosting`.`vhosts` TO 'nonpriv'@'localhost';
FLUSH PRIVILEGES;

INSERT INTO `vhosts` VALUES ('w3.myvhost1.net', NULL, 'yes', '/var/www/vhosts/01', 'www@vhost1.net', 'enable_dl=0;default_charset=koi8-r;');
INSERT INTO `vhosts` VALUES ('www.myvhost2.net', 'myvhost2.net','yes', '/var/www/vhosts/02', 'www@vhost2.net', 'default_charset=koi8-r;');

COMMIT;
