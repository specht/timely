#!/usr/bin/env python

#CREATE TABLE IF NOT EXISTS `pagecounts` (
  #`page` varchar(512) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
  #`count` int(11) NOT NULL,
  #KEY `page` (`page`)
#) ENGINE=MyISAM DEFAULT CHARSET=utf8;

import sys

for line in sys.stdin:
    if line[0:1] == '#':
        continue
    if line[0:5] != 'en.z ':
        continue
    
    page = line[5:line.index(' ', 5)]
    if (len(page) > 512):
        page = page[0:512]
        
    count = line[(5 + len(page) + 1):line.index(' ', 5 + len(page) + 1)]
    if (count != '1'):
        print("INSERT INTO `timely`.`pagecounts` (page, count) VALUES('" + page.replace("'", "''") + "', '" + count + "');")
    