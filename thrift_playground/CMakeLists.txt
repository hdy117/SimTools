cmake_minimum_required(VERSION 3.14.0)

project(thrift_playground)

set(CMAKE_CXX_STANDARD 14)

set(root_dir ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${root_dir} ${root_dir}/msg)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    add_compile_options(/MP)

    add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)
    add_definitions(-DCMAKE_TOOLCHAIN_FILE=E:/work/vcpkg/scripts/buildsystems/vcpkg.cmake)

    find_package(Thrift CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)

    include_directories(${THRIFT_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR})
    link_directories(${THRIFT_LIBRARY_DIR} ${PROTOBUF_LIBRARY_DIR} E:/work/vcpkg/installed/x64-windows/lib)

    set(lib_sys )
    set(lib_protobuf protobuf::libprotobuf)
    set(lib_log glog.lib gflags.lib)
    set(lib_rpc thrift::thrift thriftz::thriftz)
    set(lib_msg libsim_msg.lib)
else()
    include_directories(/usr/include /usr/local/include )
    link_directories(/usr/lib/x86_64-linux-gnu)

    add_compile_options(-fPIC)

    set(lib_sys pthread)
    set(lib_protobuf protobuf)
    set(lib_log glog gflags)
    set(lib_rpc thrift)
    set(lib_msg sim_msg)
endif(WIN32)

file(GLOB thrift_headers ${root_dir}/msg/gen-cpp/*.h)
file(GLOB thrift_srcs ${root_dir}/msg/gen-cpp/*.cpp)
add_library(${lib_msg} ${thrift_headers} ${thrift_srcs})

add_subdirectory(${root_dir}/greeter)

# install(TARGETS ${PROTOBUF_LIBRARIES} RUNTIME DESTINATION ${root_dir}/bin)
