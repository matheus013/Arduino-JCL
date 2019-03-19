cmake_minimum_required(VERSION 3.13)
project(jcl)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(jcl
        action.cpp
        action.h
        constants.cpp
        constants.h
        context.cpp
        context.h
        crypt.cpp
        crypt.h
        helpers.h
        jcl.cpp
        jcl.h
        library.properties
        message.cpp
        message.h
        metadata.cpp
        metadata.h
        sensor.cpp
        sensor.h
        utils.cpp
        utils.h)
