#!/usr/bin/env python

import os
import sys
import json
import subprocess

#import ycm_core

def FlagsForFile( filename ):
    flags = []
    proj_root = os.path.abspath(os.path.dirname(__file__))
    ycm_db = os.path.join(proj_root, ".ycm_db")
    rel_fname = os.path.relpath(filename, proj_root)
    print rel_fname
    with open(ycm_db, 'r') as db:
        flist = json.load(db)
        if rel_fname in flist:
            flags = flist[rel_fname]

    return {
      'flags': flags,
      'do_cache': True
    }


if __name__ == "__main__":
    print FlagsForFile(sys.argv[1])
