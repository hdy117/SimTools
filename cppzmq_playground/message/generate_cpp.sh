#!/bin/bash

root_dir=$(dirname $0)

for f in $root_dir/*.proto
do
  protoc -I=$root_dir --cpp_out=$root_dir $f
done