#!/bin/sh
#
# $Id$
#

cat <<'SQL' > vhosts.sql
CREATE DATABASE IF NOT EXISTS `hosting`;

CREATE TABLE IF NOT EXISTS `vhosts` (
  `vhost` varchar(255) NOT NULL DEFAULT '',
  `valias` varchar(255) DEFAULT '',
  `enabled` enum('yes','no') NOT NULL DEFAULT 'no',
  `rootdir` varchar(255) NOT NULL DEFAULT '',
  `admin` varchar(255) DEFAULT '',
  `extra_php_config` text,
  UNIQUE KEY `vhostname` (`vhost`,`valias`),
  KEY `enabled` (`enabled`)
) COMMENT='vhosts';

USE `hosting`;

SQL

i=1

while [ "$i" -ne "1000" ]
do
    line=$(printf "INSERT INTO \`vhosts\` VALUES ('v%d.vhosts.gazinter.net', NULL, 'yes', '/var/www/vhosts/v%d', '', '');" $i $i)
    echo "$line" >> vhosts.sql
    i=$(($i+1))
done
