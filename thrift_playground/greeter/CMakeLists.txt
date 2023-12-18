cmake_minimum_required(VERSION 3.14.0)

project(thrift_greeter)

set(cur_dir ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(thrift_greeter ${cur_dir}/client.cpp)
target_link_libraries(thrift_greeter ${lib_msg} ${lib_rpc} ${lib_protobuf} ${lib_log} ${lib_sys} )

add_executable(thrift_replier ${cur_dir}/server.cpp)
target_link_libraries(thrift_replier ${lib_msg} ${lib_rpc} ${lib_protobuf} ${lib_log} ${lib_sys} )

INSTALL(TARGETS thrift_greeter thrift_replier RUNTIME DESTINATION ${root_dir}/bin)