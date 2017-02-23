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
        src/main/cpp/include/robotkernel/bridge_interface.h
        src/main/cpp/include/robotkernel/char_ringbuffer.h
        src/main/cpp/include/robotkernel/cli_bridge.h
        src/main/cpp/include/robotkernel/cmd_delay.h
        src/main/cpp/include/robotkernel/config.h.in
        src/main/cpp/include/robotkernel/dump_log.h
        src/main/cpp/include/robotkernel/exceptions.h
        src/main/cpp/include/robotkernel/helpers.h
        src/main/cpp/include/robotkernel/interface.h
        src/main/cpp/include/robotkernel/interface_intf.h
        src/main/cpp/include/robotkernel/kernel.h
        src/main/cpp/include/robotkernel/kernel_queue.h
        src/main/cpp/include/robotkernel/kernel_worker.h
        src/main/cpp/include/robotkernel/ln_bridge.h
        src/main/cpp/include/robotkernel/log_thread.h
        src/main/cpp/include/robotkernel/loglevel.h
        src/main/cpp/include/robotkernel/module.h
        src/main/cpp/include/robotkernel/module_base.h
        src/main/cpp/include/robotkernel/module_intf.h
        src/main/cpp/include/robotkernel/rk_type.h
        src/main/cpp/include/robotkernel/rt_helper.h
        src/main/cpp/include/robotkernel/runnable.h
        src/main/cpp/include/robotkernel/service.h
        src/main/cpp/include/robotkernel/service_provider_base.h
        src/main/cpp/include/robotkernel/trigger_base.h
        src/main/cpp/char_ringbuffer.cpp
        src/main/cpp/dump_log.cpp
        src/main/cpp/exceptions.cpp
        src/main/cpp/interface.cpp
        src/main/cpp/service_provider_base.cpp
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
        src/main/cpp/service_provider_base.cpp
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