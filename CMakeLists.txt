cmake_minimum_required(VERSION 3.3)
project(robotkernel)

include_directories(
        ${STRING_UTIL_BASE}/include
        ${YAML_BASE}/include
        build/${ARCH}/include
        include/
        src/
)

set(SOURCE_FILES
        src/bridge.cpp
        src/dump_log.cpp
        src/kernel.cpp		  
        src/lttng_tp.cpp  
        src/module_base.cpp   
        src/rkc_loader.cpp   
        src/runnable.cpp	  
        src/kernel_c_wrapper.cpp  
        src/log_base.cpp     
        src/module.cpp	   
        src/service_provider.cpp  
        src/stream.cpp
        src/char_ringbuffer.cpp
        src/exceptions.cpp  
        src/log_thread.cpp   
        src/main.cpp	 
        src/rk_type.cpp      
        src/trigger.cpp
        src/helpers.cpp     
        src/kernel_worker.cpp	  
        src/process_data.cpp  
        src/robotkernel.cpp  
        src/so_file.cpp	  
        src/trigger_worker.cpp
        )
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

link_directories(${STRING_UTIL_BASE}/lib/${ARCH})
link_directories(${YAML_BASE}/lib/${ARCH})

add_executable(robotkernel ${SOURCE_FILES})
set_target_properties(robotkernel PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(robotkernel Threads::Threads ${CMAKE_DL_LIBS} string_util ln yaml-cpp)
target_include_directories(robotkernel PUBLIC src/include)

set_property(TARGET robotkernel PROPERTY CXX_STANDARD 11)
