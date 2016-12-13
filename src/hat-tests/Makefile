## This source file is part of the 'hat' open source project.
## Copyright (c) 2016, Yuriy Vosel.
## Licensed under Boost Software License.
## See LICENSE.txt for the licence information.

OBJECT_FILES_DIR = ../$(OUTPUT_DIR_NAME)/tests_obj/
EXECUTABLE = ../$(OUTPUT_DIR_NAME)/tests

SOURCES = $(shell find ./ -name "*.cpp")
OBJECT_TARGETS = $(patsubst %.cpp, $(OBJECT_FILES_DIR)%.o, $(SOURCES))

.PHONY: all clean

all: $(OBJECT_FILES_DIR) $(OBJECT_TARGETS)
	$(CC) $(CXX_FLAGS) $(OBJECT_TARGETS) -o $(EXECUTABLE)

$(OBJECT_FILES_DIR):
	mkdir -p $@

$(OBJECT_FILES_DIR)%.o: %.cpp
	$(CC) $(CXX_FLAGS) -D HAT_CORE_HEADERONLY_MODE -c $< -o $@

clean:
	rm -rf $(OBJECT_FILES_DIR)
	rm -f $(EXECUTABLE)
