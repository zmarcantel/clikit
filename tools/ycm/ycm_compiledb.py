#!/usr/bin/env python

import os
import sys
import json

if len(sys.argv) != 2:
    print "expected 1 argument [clang compile database]"
    sys.exit(1)

file_flags = {}

with open(sys.argv[1], 'r') as db:
    dbjs = json.load(db)
    for f in dbjs:
        file_flags[f["file"]] = f["command"].split(" ")[1:]


dest_dir = os.path.dirname(sys.argv[1])
with open(os.path.join(dest_dir, ".ycm_db"), 'w+') as db:
    db.truncate()
    json.dump(file_flags, db, indent=4, sort_keys=True)
