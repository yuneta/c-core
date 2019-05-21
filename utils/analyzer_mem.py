#!/usr/bin/python

import json
import re

info_line = re.compile('DEBUG: (?P<dictio>.+)')

allocs_frees = {}
original_size = {}
original_size_times = {}

dict_memtrace = {}

def main(filename):
    print("analyzing %s" % filename)
    i = 0
    with open(filename) as f:
        for line in f:
            i += 1
            m = re.match(info_line, line)
            if not m:
                continue
            try:
                logmsg = json.loads(m.group('dictio'))
            except:
                #print("Fail in line %d: %s" % (i, line))
                continue
            if not logmsg['msgset'] == 'Statistics':
                continue

            if logmsg['msg'] == 'alloc memory':
                pointer = logmsg['pointer']
                if pointer in allocs_frees:
                    allocs_frees[pointer] += 1
                else:
                    allocs_frees[pointer] = 1
                osize = logmsg['original_size']
                original_size[pointer] = osize
                if osize in original_size_times:
                    original_size_times[osize] += 1
                else:
                    original_size_times[osize] = 1
                memtrace = logmsg['memtrace']
                dict_memtrace[memtrace] = 1

    print("%d entries" % len(allocs_frees))


    i = 0
    with open(filename) as f:
        for line in f:
            i += 1
            m = re.match(info_line, line)
            if not m:
                continue
            try:
                logmsg = json.loads(m.group('dictio'))
            except:
                #print("Fail in line %d: %s" % (i, line))
                continue
            if not logmsg['msgset'] == 'Statistics':
                continue
            if logmsg['msg'] == 'free memory':
                pointer = logmsg['pointer']
                if pointer in allocs_frees:
                    allocs_frees[pointer] -= 1
                else:
                    print("pointer not allocated %s" % pointer)
                memtrace = logmsg['memtrace']
                try:
                    del dict_memtrace[memtrace]
                except:
                    print("memtrace %s not found" % memtrace)


    losts = {}
    for pointer in allocs_frees:
        counter = allocs_frees[pointer]
        if(counter == 0):
            continue
        losts[pointer] = counter


    #for pointer in sorted(losts):
        #print("%s = %d, original_size %s, times %d" % (
            #pointer,
            #losts[pointer],
            #original_size[pointer],
            #original_size_times[original_size[pointer]]
        #))

    print("%d lost" % len(losts))

    for pointer in sorted(losts, reverse=False):
        with open(filename) as f:
            for line in f:
                if re.findall(pointer, line):
                    #print(line)
                    pass
        #break

    print("memtrace:")
    print("%r" % dict_memtrace)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("filename")
    args = parser.parse_args()
    main(args.filename)
