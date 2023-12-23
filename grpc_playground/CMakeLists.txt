cmake_minimum_required(VERSION 3.14.0)

project(grpc_playground)

set(CMAKE_CXX_STANDARD 14)

set(root_dir ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${root_dir} ${root_dir}/msg)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    add_compile_options(/MP)

    set(vcpkg_dir E:/work/vcpkg)

    add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)
    add_definitions(-DCMAKE_TOOLCHAIN_FILE=${vcpkg_dir}/scripts/buildsystems/vcpkg.cmake)

    find_package(gRPC CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)

    include_directories(${GRPC_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR}  ${vcpkg_dir}/installed/x64-windows/include)
    link_directories(${GRPC_LIBRARY_DIR} ${PROTOBUF_LIBRARY_DIR} ${vcpkg_dir}/installed/x64-windows/lib)
    
    message(STATUS "protobuf inc_dir:${PROTOBUF_INCLUDE_DIR}")

    set(lib_sys )
    set(lib_protobuf protobuf::libprotobuf)
    set(lib_log glog.lib gflags.lib)
    set(lib_grpc gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
    set(lib_msg libsim_msg.lib)
else()
    include_directories(/usr/include /usr/local/include )
    link_directories(/usr/lib/x86_64-linux-gnu)

    add_compile_options(-fPIC)

    set(lib_sys pthread)
    set(lib_protobuf protobuf)
    set(lib_log glog gflags)
    set(lib_grpc grpc++ grpc++_unsecure grpc)
    set(lib_msg sim_msg)
endif(WIN32)

file(GLOB proto_headers ${root_dir}/msg/*.pb.h)
file(GLOB proto_srcs ${root_dir}/msg/*.pb.cc)
add_library(${lib_msg} ${proto_headers} ${proto_srcs})

add_subdirectory(${root_dir}/greeter)

# install(TARGETS ${PROTOBUF_LIBRARIES} RUNTIME DESTINATION ${root_dir}/bin)
