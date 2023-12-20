#!/bin/bash

root_dir=$(pwd)

cd $root_dir/msg && bash ./generate_cpp.sh

mkdir -p $root_dir/build

cd $root_dir/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && make install