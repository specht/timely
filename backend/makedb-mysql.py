#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import csv

pageCounts = dict()

with open('../wp-stats/pagecounts.csv') as f:
    for row in csv.reader(f):
        pageCounts[row[0]] = int(row[1])
        
with open('timely-mysql.sql', 'w') as fout:
    # write schema
    fout.write("CREATE TABLE IF NOT EXISTS `events` (\n\
  `id` int(11) NOT NULL AUTO_INCREMENT,\n\
  `t` int(11) NOT NULL,\n\
  `relevance` int(11) NOT NULL,\n\
  `res` int(11) NOT NULL,\n\
  `fiction` int(11) NOT NULL,\n\
  `type` char(1) NOT NULL,\n\
  `content` text NOT NULL,\n\
  `path` varchar(255) NOT NULL,\n\
  PRIMARY KEY (`id`),\n\
  KEY `index_events_t` (`t`),\n\
  KEY `index_events_relevance` (`relevance`),\n\
  FULLTEXT KEY `index_events_content` (`content`)\n\
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;\n")
    
    with open('timely/timely-2012-01-10.txt', 'r') as fin:
        for line in fin:
            t = line[0:line.index(' ')]
            line = line[line.index(' ') + 1:]
            res = line[0:line.index(' ')]
            line = line[line.index(' ') + 1:]
            fiction = line[0:line.index(' ')]
            line = line[line.index(' ') + 1:]
            v_type = line[0:line.index(' ')]
            line = line[line.index(' ') + 1:]
            content = line.strip()
            path = ''
            if '<!--' in line:
                content = line[0:line.rindex('<!--')].strip()
                path = line[line.rindex('<!--'):].replace('<!--', '').replace('-->', '').strip()
                path = path[0:path.rindex(' ')].strip()
            
            # now determine relevance of the event
            p = -1
            pages = set()
            while True:
                p = content.find('http://en.wikipedia.org/wiki/', p + 1)
                if p < 0:
                    break
                page = line[p:line.index('"', p)][29:]
                pages.add(page)
            maxRelevance = 0
            for page in pages:
                if page in pageCounts:
                    relevance = pageCounts[page];
                    maxRelevance += relevance
            if len(pages) > 0:
                maxRelevance /= len(pages)
            
            fout.write("INSERT INTO events (t, relevance, res, fiction, type, content, path) VALUES (" + 
                t + ", " + str(maxRelevance) + ", " + res + ", " + ('1' if fiction == 'f' else '0') + ", '" + v_type + "', '" +
                content.replace("'", "''") + "', '" + path + "');\n")
            