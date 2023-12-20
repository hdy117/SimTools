cmake_minimum_required(VERSION 3.14.0)

project(msg_gen)

find_package(absl CONFIG REQUIRED PATHS /opt/grpc)
find_package(utf8_range CONFIG REQUIRED PATHS /opt/grpc)
find_package(Protobuf CONFIG REQUIRED PATHS /opt/grpc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_files)

add_library(msg_gen SHARED ${src_files} ../msg/person.pb.cc)

target_link_directories(msg_gen PRIVATE /opt/grpc/lib)
target_link_libraries(msg_gen PRIVATE protobuf::libprotobuf pthread)
target_include_directories(msg_gen PRIVATE ../ /opt/grpc/include)
