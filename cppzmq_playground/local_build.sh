#!/bin/bash

root_dir=$(pwd)
build_dir=$root_dir/build

rm -rf $build_dir && mkdir -p $build_dir

cd $build_dir && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && make install