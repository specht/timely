#!/usr/bin/env python

import sys

pages = set()

with open('../backend/timely/timely-2012-01-10.txt') as f:
    for line in f:
        p = -1
        while True:
            p = line.find('http://en.wikipedia.org/wiki/', p + 1)
            if p < 0:
                break
            page = line[p:line.index('"', p)][29:]
            pages.add(page)
                
#print("Got " + str(len(pages)) + " page references.")

pageCount = dict()

with open('pagecounts-2011-08-all-en.txt') as f:
    for line in f:
        if line[0:1] == '#':
            continue
        if line[0:5] != 'en.z ':
            continue
        
        page = line[5:line.index(' ', 5)]
        if page not in pages:
            continue
        
        count = line[(5 + len(page) + 1):line.index(' ', 5 + len(page) + 1)]
        pageCount[page] = count
        print("\"" + page + "\"," + count)

