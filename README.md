bazel-bootstrap
===============

C++ defaults
------------

This is set in `tools/bazel.rc`

- C++14
- enable all errors except unused functions
- all warnings are errors


YouCompleteMe
-------------

To initialize, run: `tools/compiledb.sh`

This will generate two files in the project root:

1. compile_commands.json
    - this is the typical `clang`-compatible compile database
2. .ycm_db
    - this is what the `.ycm_extra_conf.py` uses to extract per-file flags

When you add files, you need to reinitialize. It is a good idea to add it to a Bazel
event listener or as part of your build.


__NOTE:__ gtest (and other external deps) includes will not work until bazel has fetched the sources.


gtest
-----

Pulls from github on a tagged release.

Configurable version to use in `WORKSPACE`.
