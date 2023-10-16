cmake_minimum_required(VERSION 3.6)

project(Mute)

set(SOURCE_FILES
	src/main.h src/main.cpp
	src/message_overrides.h src/message_overrides.cpp
)

add_library(${PROJECT_NAME} SHARED)
add_subdirectory(mmlib)
mmlib_default_setup()