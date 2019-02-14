#!/usr/bin/env bash

set -e

RELEASE_VERSION=0.3.0
SCRIPT_PATH=$(dirname "$0")/ycm
BAZELYCM_DIR=$SCRIPT_PATH/.bazel_ycm

if [ ! -d "$BAZELYCM_DIR" ]; then
    mkdir -p $BAZELYCM_DIR
    pushd $BAZELYCM_DIR
    curl -L https://github.com/grailbio/bazel-compilation-database/archive/$RELEASE_VERSION.tar.gz | tar -xz --strip 1
    popd

    patch $BAZELYCM_DIR/generate.sh < $SCRIPT_PATH/compdb_relpath.diff
    patch $BAZELYCM_DIR/aspects.bzl < $SCRIPT_PATH/compdb_nofilename.diff
    patch $BAZELYCM_DIR/aspects.bzl < $SCRIPT_PATH/compdb_undefined_variable.diff

    touch $BAZELYCM_DIR/BUILD
fi

$BAZELYCM_DIR/generate.sh
