cmake_minimum_required(VERSION 3.14.0)

project(proto_test)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_files)

add_executable(proto_test ${src_files} ../msg/person.pb.cc)
target_link_directories(proto_test PRIVATE /usr/lib/x86_64-linux-gnu ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(proto_test PRIVATE /usr/include ../)
target_link_libraries(proto_test protobuf pthread msg_gen)