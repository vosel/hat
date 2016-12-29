## This source file is part of the 'hat' open source project.
## Copyright (c) 2016, Yuriy Vosel.
## Licensed under Boost Software License.
## See LICENSE.txt for the licence information.

export CC = g++ 
export CXX_FLAGS = -std=c++14 
export OUTPUT_DIR_NAME = bin

.PHONY: tests tool all robotlib clean

all: robotlib tests tool 

robotlib:
	cd ./external_dependencies/robot && $(MAKE) build

tests:
	cd ./hat-tests && $(MAKE) all

tool:
	cd ./hat && $(MAKE) all
	
clean:
	cd ./hat && $(MAKE) clean
	cd ./hat-tests && $(MAKE) clean
	rm -d $(OUTPUT_DIR_NAME)