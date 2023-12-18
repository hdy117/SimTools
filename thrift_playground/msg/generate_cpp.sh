#!/bin/bash

root_dir=$(dirname $0)

for f in $root_dir/*.thrift
do
  thrift --gen cpp $f
done