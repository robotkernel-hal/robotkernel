cmake_minimum_required(VERSION 3.3)
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VER
    RESULT_VARIABLE GIT_RESULT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(GIT_RESULT EQUAL 0)
    string(REGEX MATCH "v?([0-9]+\\.[0-9]+\\.[0-9]+)" _ ${GIT_VER})
    set(PROJECT_VERSION ${CMAKE_MATCH_1})
endif()

if(NOT PROJECT_VERSION)
    set(PROJECT_VERSION 0.0.0)
endif()
project(robotkernel VERSION ${PROJECT_VERSION})

include_directories(
        ${YAML_BASE}/include
        build/${ARCH}/include
        include/
        src/
)

FILE(GLOB SOURCE_FILES src/*.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(robotkernel_service_helper REQUIRED)

include_directories(${yaml-cpp_INCLUDE_DIRS} yaml-cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(ExternalProject)
include(CMakePackageConfigHelpers)

set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(VERSION "${PROJECT_VERSION}")
set(PACKAGE_URL "${CMAKE_PROJECT_HOMEPAGE_URL}")
set(PACKAGE "${CMAKE_PROJECT_NAME}")
set(PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(PACKAGE_TARNAME "${CMAKE_PROJECT_NAME}")
set(PACKAGE_STRING "${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION}")

check_include_files("arpa/inet.h" HAVE_ARPA_INET_H)
check_symbol_exists("clock_gettime" "time.h" HAVE_CLOCK_GETTIME)
check_include_files("dlfcn.h" HAVE_DLFCN_H)
check_include_files("fcntl.h" HAVE_FCNTL_H)
check_include_files("inttypes.h" HAVE_INTTYPES_H)
find_library(HAS_RT_RAW rt)
if(HAS_RT_RAW)
    SET(HAVE_LIBRT 1)
endif()
check_include_files("limits.h" HAVE_LIMITS_H)
check_symbol_exists("malloc" "stdlib.h" HAVE_MALLOC)
check_include_files("memory.h" HAVE_MEMORY_H)
check_symbol_exists("memset" "string.h" HAVE_MALLOC)
check_include_files("net/bpf.h" HAVE_NET_BPF_H)
check_include_files("net/if.h" HAVE_NET_IF_H)
check_include_files("netinet/in.h" HAVE_NETINET_IN_H)
check_include_files("pthread.h" HAVE_PTHREAD)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists("pthread_setaffinity_np" "pthread.h" HAVE_PTHREAD_SETAFFINITY_NP)
check_symbol_exists("realloc" "stdlib.h" HAVE_REALLOC)
check_symbol_exists("socket" "sys/socket.h" HAVE_SOCKET)
check_include_files("stdint.h" HAVE_STDINT_H)
check_include_files("stdlib.h" HAVE_STDLIB_H)
check_symbol_exists("strdup" "string.h" HAVE_STRDUP)
check_symbol_exists("strerror" "string.h" HAVE_STRERROR)
check_include_files("strings.h" HAVE_STRINGS_H)
check_include_files("string.h" HAVE_STRING_H)
check_symbol_exists("strndup" "string.h" HAVE_STRNDUP)
check_include_files("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_files("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_files("sys/stat.h" HAVE_SYS_STAT_H)
check_include_files("sys/time.h" HAVE_SYS_TIME_H)
check_include_files("sys/types.h" HAVE_SYS_TYPES_H)
check_include_files("unistd.h" HAVE_UNISTD_H)

set(SERVICE_DEFINITION_HEADER include/robotkernel/service_definitions.h)
set(SERVICE_DEFINITIONS 
    robotkernel/module/set_state
    robotkernel/module/get_state
    robotkernel/module/get_config
    robotkernel/kernel/get_dump_log
    robotkernel/kernel/config_dump_log
    robotkernel/kernel/add_module
    robotkernel/kernel/remove_module
    robotkernel/kernel/module_list
    robotkernel/kernel/reconfigure_module
    robotkernel/kernel/list_devices
    robotkernel/kernel/process_data_info
    robotkernel/kernel/trigger_info
    robotkernel/kernel/stream_info
    robotkernel/kernel/service_interface_info
    robotkernel/kernel/add_pd_injection
    robotkernel/kernel/del_pd_injection
    robotkernel/kernel/list_pd_injections
    robotkernel/log_base/configure_loglevel)

service_generate("services" ${CMAKE_SOURCE_DIR}/share/service_definitions ${SERVICE_DEFINITION_HEADER}  ${SERVICE_DEFINITIONS})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/robotkernel/config.h)

add_executable(robotkernel ${SOURCE_FILES} ${SERVICE_DEFINITION_HEADER})
set_target_properties(robotkernel PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(robotkernel Threads::Threads ${CMAKE_DL_LIBS} ${yaml-cpp_LIBS})
target_include_directories(robotkernel PUBLIC src/include)

set_property(TARGET robotkernel PROPERTY CXX_STANDARD 11)
