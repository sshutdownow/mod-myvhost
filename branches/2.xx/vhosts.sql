/*
* Copyright (c) 2005-2007 Igor Popov <igorpopov@newmail.ru> 
*
* $Id$
*
*/

CREATE DATABASE `hosting`;
USE `hosting`;

CREATE TABLE `vhosts` (
    `vhost`   varchar(255) NOT NULL default '',
    `enabled` enum('yes','no') NOT NULL default 'no',
    `rootdir` varchar(255) NOT NULL default '',
    `admin`   varchar(255) default '',
    `extra_php_config` text,
    UNIQUE KEY `vhostname` (`vhost`),
    KEY `enabled` (`enabled`)
) TYPE=MyISAM COMMENT='vhosts';
		  
GRANT SELECT ON `hosting`.`vhosts` TO 'nonpriv'@'localhost' IDENTIFIED BY 'M3Ga PaSsVVd';

INSERT INTO `vhosts` VALUES ('w3.myvhost1.net', 'yes', '/usr/local/www/vhosts/01', 'www@vhost1.net', 'enable_dl=0;default_charset=koi8-r');
INSERT INTO `vhosts` VALUES ('www.myvhost2.net', 'yes', '/usr/local/www/vhosts/02', 'www@vhost2.net', 'default_charset=koi8-r');

COMMIT;
