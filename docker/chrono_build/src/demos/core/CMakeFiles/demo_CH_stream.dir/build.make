# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /opt/chrono/chrono_source/chrono

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /opt/chrono/chrono_build

# Include any dependencies generated for this target.
include src/demos/core/CMakeFiles/demo_CH_stream.dir/depend.make

# Include the progress variables for this target.
include src/demos/core/CMakeFiles/demo_CH_stream.dir/progress.make

# Include the compile flags for this target's objects.
include src/demos/core/CMakeFiles/demo_CH_stream.dir/flags.make

src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o: src/demos/core/CMakeFiles/demo_CH_stream.dir/flags.make
src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o: /opt/chrono/chrono_source/chrono/src/demos/core/demo_CH_stream.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/opt/chrono/chrono_build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o"
	cd /opt/chrono/chrono_build/src/demos/core && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o -c /opt/chrono/chrono_source/chrono/src/demos/core/demo_CH_stream.cpp

src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.i"
	cd /opt/chrono/chrono_build/src/demos/core && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /opt/chrono/chrono_source/chrono/src/demos/core/demo_CH_stream.cpp > CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.i

src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.s"
	cd /opt/chrono/chrono_build/src/demos/core && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /opt/chrono/chrono_source/chrono/src/demos/core/demo_CH_stream.cpp -o CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.s

src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.requires:

.PHONY : src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.requires

src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.provides: src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.requires
	$(MAKE) -f src/demos/core/CMakeFiles/demo_CH_stream.dir/build.make src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.provides.build
.PHONY : src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.provides

src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.provides.build: src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o


# Object files for target demo_CH_stream
demo_CH_stream_OBJECTS = \
"CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o"

# External object files for target demo_CH_stream
demo_CH_stream_EXTERNAL_OBJECTS =

bin/demo_CH_stream: src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o
bin/demo_CH_stream: src/demos/core/CMakeFiles/demo_CH_stream.dir/build.make
bin/demo_CH_stream: lib64/libChronoEngine.so
bin/demo_CH_stream: src/demos/core/CMakeFiles/demo_CH_stream.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/opt/chrono/chrono_build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../../bin/demo_CH_stream"
	cd /opt/chrono/chrono_build/src/demos/core && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/demo_CH_stream.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/demos/core/CMakeFiles/demo_CH_stream.dir/build: bin/demo_CH_stream

.PHONY : src/demos/core/CMakeFiles/demo_CH_stream.dir/build

src/demos/core/CMakeFiles/demo_CH_stream.dir/requires: src/demos/core/CMakeFiles/demo_CH_stream.dir/demo_CH_stream.cpp.o.requires

.PHONY : src/demos/core/CMakeFiles/demo_CH_stream.dir/requires

src/demos/core/CMakeFiles/demo_CH_stream.dir/clean:
	cd /opt/chrono/chrono_build/src/demos/core && $(CMAKE_COMMAND) -P CMakeFiles/demo_CH_stream.dir/cmake_clean.cmake
.PHONY : src/demos/core/CMakeFiles/demo_CH_stream.dir/clean

src/demos/core/CMakeFiles/demo_CH_stream.dir/depend:
	cd /opt/chrono/chrono_build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /opt/chrono/chrono_source/chrono /opt/chrono/chrono_source/chrono/src/demos/core /opt/chrono/chrono_build /opt/chrono/chrono_build/src/demos/core /opt/chrono/chrono_build/src/demos/core/CMakeFiles/demo_CH_stream.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/demos/core/CMakeFiles/demo_CH_stream.dir/depend

