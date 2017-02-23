cmake_minimum_required(VERSION 3.3)
project(robotkernel)

include_directories(
        ${STRING_UTIL_BASE}/include
        ${LN_BASE}/include
        ${BOOST_BASE}/include
        ${YAML_BASE}/include
        
        build/${ARCH}/src/main/cpp/include
        src/main/cpp/include
        src/main/cpp
)

set(SOURCE_FILES
        src/main/cpp/char_ringbuffer.cpp
        src/main/cpp/dump_log.cpp
        src/main/cpp/exceptions.cpp
        src/main/cpp/interface.cpp
        src/main/cpp/interface_base.cpp
        src/main/cpp/kernel.cpp
        src/main/cpp/kernel_worker.cpp
        src/main/cpp/ln_bridge.cpp
        src/main/cpp/log_thread.cpp
        src/main/cpp/main.cpp
        src/main/cpp/module.cpp
        src/main/cpp/module_base.cpp
        src/main/cpp/module_intf.cpp
        src/main/cpp/runnable.cpp
        src/main/cpp/rt_helper.cpp
        src/main/cpp/rk_type.cpp
        src/main/cpp/cli_bridge.cpp
        src/main/cpp/cli_server.h
        src/main/cpp/cli_server.cpp
        src/main/cpp/trigger_base.cpp)

set(TEST_CLISERVER_SOURCE_FILES
        src/main/cpp/char_ringbuffer.cpp
        src/main/cpp/dump_log.cpp
        src/main/cpp/exceptions.cpp
        src/main/cpp/interface.cpp
        src/main/cpp/interface_base.cpp
        src/main/cpp/kernel.cpp
        src/main/cpp/kernel_worker.cpp
        src/main/cpp/ln_bridge.cpp
        src/main/cpp/log_thread.cpp
        src/main/cpp/module.cpp
        src/main/cpp/module_base.cpp
        src/main/cpp/module_intf.cpp
        src/main/cpp/runnable.cpp
        src/main/cpp/rt_helper.cpp
        src/main/cpp/cli_server.h
        src/main/cpp/cli_server.cpp
        src/main/cpp/trigger_base.cpp
        src/main/cpp/rk_type.cpp
        src/main/cpp/cli_bridge.cpp
        src/test/cpp/test_cli_server.cpp
        )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

link_directories(${STRING_UTIL_BASE}/lib/${ARCH})
link_directories(${LN_BASE}/lib/${ARCH})
link_directories(${YAML_BASE}/lib/${ARCH})

add_executable(robotkernel ${SOURCE_FILES})
set_target_properties(robotkernel PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(robotkernel Threads::Threads ${CMAKE_DL_LIBS} string_util ln yaml-cpp)
target_include_directories(robotkernel PUBLIC src/main/cpp/include)

add_executable(CliServerTest ${TEST_CLISERVER_SOURCE_FILES})
target_link_libraries(CliServerTest Threads::Threads ${CMAKE_DL_LIBS} string_util ln yaml-cpp robotkernel)
target_include_directories(CliServerTest PUBLIC src/main/cpp/include)