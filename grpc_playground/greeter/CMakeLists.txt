cmake_minimum_required(VERSION 3.14.0)

project(grpc_greeter)

set(cur_dir ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(grpc_greeter ${cur_dir}/client.cpp)
target_link_libraries(grpc_greeter ${lib_msg} ${lib_grpc} ${lib_protobuf} ${lib_log} ${lib_sys} )

add_executable(grpc_replier ${cur_dir}/server.cpp)
target_link_libraries(grpc_replier ${lib_msg} ${lib_grpc} ${lib_protobuf} ${lib_log} ${lib_sys} )

INSTALL(TARGETS grpc_greeter grpc_replier RUNTIME DESTINATION ${root_dir}/bin)