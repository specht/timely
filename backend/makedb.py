#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

with open('temp-statements.sql', 'w') as fout:
    # write schema
    
    fout.write("CREATE TABLE 'events' ( \
        't' INTEGER NOT NULL, \
        'res' INTEGER NOT NULL, \
        'fiction' INTEGER NOT NULL, \
        'type' INTEGER NOT NULL, \
        'content' TEXT NOT NULL, \
        'path' TEXT NOT NULL \
    );")

    fout.write('BEGIN;\n')
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
            fout.write("INSERT INTO events VALUES (" + 
                t + ", " + res + ", " + ('1' if fiction == 'f' else '0') + ", '" + v_type + "', '" +
                content.replace("'", "''") + "', '" + path + "');\n")
            
    fout.write('COMMIT;\n')
    
    # create index
    fout.write('BEGIN;\n')
    for x in ['t', 'res', 'fiction', 'type', 'path']:
        fout.write("CREATE INDEX events_index_" + x + " on events(" + x + ");\n")
        
    fout.write('COMMIT;\n')
    
if os.path.isfile('timely.db'):
    os.system('rm timely.db')
    
os.system("sqlite3 -init temp-statements.sql timely.db \"\"")
os.system('rm temp-statements.sql')
