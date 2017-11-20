#!/usr/bin/env python

import os
import sys
import json
import subprocess

#import ycm_core

def FlagsForFile( filename ):
    flags = []
    proj_root = os.path.abspath(os.path.dirname(__file__))
    proj_name = os.path.basename(proj_root)
    ycm_db = os.path.join(proj_root, ".ycm_db")
    rel_fname = os.path.relpath(filename, proj_root)
    with open(ycm_db, 'r') as db:
        flist = json.load(db)
        if rel_fname in flist:
            flags = flist[rel_fname]

    for (i, f) in enumerate(flags):
        # abspath all bazel build directories
        bazel_prefix = "bazel-"
        if f[0:len(bazel_prefix)] == bazel_prefix:
            flags[i] = os.path.join(proj_root, f)
            continue

        # abspath the bazel external includes
        external_prefix = "external/"
        if f[0:len(external_prefix)] == external_prefix:
            flags[i] = os.path.join(proj_root, os.path.join("bazel-{}".format(proj_name), f))
            continue


    return {
      'flags': flags,
      'do_cache': True
    }


if __name__ == "__main__":
    print FlagsForFile(sys.argv[1])
