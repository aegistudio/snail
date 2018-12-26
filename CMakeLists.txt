# @Snail@ - Code Execution Footprint Tracer & Explorer.
#
# Copyright (C) 2018 Haoran Luo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# The tool Snail comprises of two parts:
# - Tracer: aims at creating a runtime footprint (which lines of codes are
# executed, and how are the local variables being altered) of a program or 
# function.
# - Explorer: aims at providing an explorer that demonstrate the execution 
# footprints in an easy-to-comprehend and user-friendly way.
cmake_minimum_required(VERSION 3.5)
project(snail)

# Configure C++ standard to use C++11.
set(CMAKE_CXX_STANDARD 11)

# The partition of configuring the build of the snail.
option(BUILD_VIEWER "Whether the snail viewer will be built." ON)
if(BUILD_VIEWER) # Begin BUILD_VIEWER

# Make the include directory to be included by other modules.
include_directories("include")

# Make sure that the python is installed on the target machine.
find_package(PythonInterp REQUIRED)
if(NOT ${PYTHONINTERP_FOUND})
message(SEND_ERROR "Python must be installed to amalgmate JsonCpp.")
endif()

# Amalgamate the JsonCpp for json include.
set(AMALGAMATED_JSONCPP_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
set(AMALGAMATED_JSONCPP_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/src/jsoncpp.cpp")
add_custom_command(OUTPUT ${AMALGAMATED_JSONCPP_SOURCE} COMMAND 
	"${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/amalgamate.py"
	"--source=${AMALGAMATED_JSONCPP_SOURCE}" 
	"--top=${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp"
	"--include=${AMALGAMATED_JSONCPP_INCLUDE_DIR}/json/json.h"
	COMMENT "Amalgamating JsonCpp: ${AMALGAMATED_JSONCPP_SOURCE}." VERBATIM)
include_directories(${AMALGAMATED_JSONCPP_INCLUDE_DIR})

# Ensure that Curses (or actually NCurses) is installed and configured.
find_package(Curses REQUIRED)
if(${CURSES_FOUND})
include_directories(${CURSES_INCLUDE_DIRS})
else()
message(SEND_ERROR "Curses (or NCurses) must be installed for UI build-up.")
endif()

# Build the viewer executable.
add_executable(snailviewer
	"${CMAKE_CURRENT_SOURCE_DIR}/src/snailviewer/main.cpp"
	"${AMALGAMATED_JSONCPP_SOURCE}")
target_link_libraries(snailviewer ${CURSES_LIBRARIES} Threads::Threads)
	
endif() # End BUILD_VIEWER
