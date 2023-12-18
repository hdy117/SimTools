#!/bin/bash

root_dir=$(dirname $0)

for f in $root_dir/*.proto
do
  protoc -I=$root_dir --cpp_out=$root_dir $f
done

GRPC_CPP_PLUGIN=`which grpc_cpp_plugin`
protoc -I=$root_dir --grpc_out=$root_dir --plugin=protoc-gen-grpc=$GRPC_CPP_PLUGIN $root_dir/HelloService.proto