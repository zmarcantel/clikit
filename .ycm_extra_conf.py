#!/usr/bin/env python

import os
import sys
import json
import subprocess

#import ycm_core

def make_ycm_db(proj_root):
    subprocess.check_call([
        os.path.join(proj_root, "external", "tools", "compiledb.sh")
    ])

def get_db_flags(proj_root, rel_fname, final=False):
    ycm_db = os.path.join(proj_root, "compile_commands.json")

    # if no db file, make one
    if not os.path.isfile(ycm_db):
        make_ycm_db(proj_root)

    with open(ycm_db, 'r') as db:
        cmdlist = json.load(db)

        for desc in cmdlist:
            if desc['file'] == rel_fname:
                return desc['command'].split(" ")

    if final:
        raise Exception("could not find file flags even after regenerating")

    # newly analyzed file so rescan and recurse
    make_ycm_db(proj_root)
    return get_db_flags(proj_root, rel_fname, final=True)

def FlagsForFile( filename ):
    proj_root = os.path.abspath(os.path.dirname(__file__))
    proj_name = os.path.basename(proj_root)
    rel_fname = os.path.relpath(filename, proj_root)

    flags = get_db_flags(proj_root, rel_fname)
    if flags is not None:
        for (i, f) in enumerate(flags):
            # abspath all bazel build directories
            if f.startswith("bazel-"):
                flags[i] = os.path.join(proj_root, f)
                continue

            # abspath the bazel external includes
            if f.startswith("external/"):
                flags[i] = os.path.join(proj_root, os.path.join("bazel-{}".format(proj_name), f))
                continue

    else:
        flags = [] # empty.... TODO: error here instead?


    return {
      'flags': flags,
      'do_cache': True
    }


if __name__ == "__main__":
    print FlagsForFile(sys.argv[1])
