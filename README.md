[![BuildBot Status](http://rmc-chimaere:8010/badge.png?builder=robotkernel/robotkernel&branch=master)](http://rmc-chimaere:8010/builders/robotkernel%2Frobotkernel)

# README.md for robotkernel

## build robotkernel

This directory contains all the source code needed to build the robotkernel 
and serveral modules.

To generate the build system execute
  
    autoreconf -if
    
To build the code execute

    ./configure
    make

## devices

robotkernel::device base class 

all devices share a unique device_name and an owner string

* trigger_device
* process_data_device
* bridge_device
* service_provider_device

