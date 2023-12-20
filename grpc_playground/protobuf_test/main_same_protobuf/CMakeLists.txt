cmake_minimum_required(VERSION 3.14.0)

project(proto_test)

find_package(absl CONFIG REQUIRED PATHS /opt/grpc)
find_package(utf8_range CONFIG REQUIRED PATHS /opt/grpc)
find_package(Protobuf CONFIG REQUIRED PATHS /opt/grpc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_files)

add_executable(proto_test ${src_files} ../msg/person.pb.cc)

target_link_directories(proto_test PRIVATE /opt/grpc/lib ./)
target_link_libraries(proto_test PRIVATE protobuf::libprotobuf pthread msg_gen)
target_include_directories(proto_test PRIVATE ../ /opt/grpc/include)