#!/bin/bash

root_dir=$(pwd)

mkdir -p $root_dir/build

cd $root_dir/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && make install