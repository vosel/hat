## This source file is part of the 'hat' open source project.
## Copyright (c) 2016, Yuriy Vosel.
## Licensed under Boost Software License.
## See LICENSE.txt for the licence information.

OBJECT_FILES_DIR = ../$(OUTPUT_DIR_NAME)/tool_obj/
EXECUTABLE = ../$(OUTPUT_DIR_NAME)/hat

CXX_ADDITIONAL_FLAGS = -D HAT_CORE_HEADERONLY_MODE
CXX_ADDITIONAL_FLAGS_FOR_TAU = -D TAU_HEADERONLY -I ../external_dependencies/tau/src/cpp 
CXX_ADDITIONAL_FLAGS_FOR_BOOST_LIBS = -lboost_system -pthread -lboost_thread -lboost_program_options 
CXX_ADDITIONAL_FLAGS_FOR_ROBOT_LIBS = -lrt -lX11 -lXtst -lXinerama 
ROBOT_STATIC_LIBRARY = ../external_dependencies/robot/Binaries/Linux/libRobot.a

SOURCES = $(shell find ./ -name "*.cpp")
OBJECT_TARGETS = $(patsubst %.cpp, $(OBJECT_FILES_DIR)%.o, $(SOURCES))

.PHONY: all clean

all: $(OBJECT_FILES_DIR) $(OBJECT_TARGETS)
	$(CC) $(CXX_FLAGS) $(CXX_ADDITIONAL_FLAGS_FOR_BOOST_LIBS) $(CXX_ADDITIONAL_FLAGS_FOR_ROBOT_LIBS) $(OBJECT_TARGETS) $(ROBOT_STATIC_LIBRARY) -o $(EXECUTABLE)

$(OBJECT_FILES_DIR):
	mkdir -p $@

$(OBJECT_FILES_DIR)%.o: %.cpp
	$(CC) $(CXX_FLAGS) $(CXX_ADDITIONAL_FLAGS) $(CXX_ADDITIONAL_FLAGS_FOR_TAU) -c $< -o $@

clean:
	rm -rf $(OBJECT_FILES_DIR)
	rm -f $(EXECUTABLE)
