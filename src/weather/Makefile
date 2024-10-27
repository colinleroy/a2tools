# Makefile For FujiNet Weather
# 2024
#


TARGETS = apple2enh apple2
PROGRAM := weather

# Set DEBUG to true or false to see more or less output about what is going on.
DEBUG := true
export DEBUG

# Edit below to point to a folder that has the apple-tools AppleComander and bitsy.bye script
# which can be this repo itself if you have it on your system, otherwise point to the apple-tools
# folder you have on your system.
export FUJINET_BUILD_TOOLS_DIR := ../../fujinet-build-tools

# Set this to the version of FN-LIB you wish to use in this project:
export FUJINET_LIB_VERSION := 4.7.3

SUB_TASKS := clean disk test release
.PHONY: all help $(SUB_TASKS)

all:
	@for target in $(TARGETS); do \
		echo "-------------------------------------"; \
		echo "Building $$target"; \
		echo "-------------------------------------"; \
		$(MAKE) --no-print-directory -f ../makefiles/build.mk CURRENT_TARGET=$$target PROGRAM=$(PROGRAM) $(MAKECMDGOALS); \
	done
	
# if disk images were built show them
	@if [ -d ./dist ]; then \
		echo "Contents of dist:"; \
		ls -1 ./dist; \
	fi

$(SUB_TASKS): _do_all
$(SUB_TASKS):
	@:

_do_all: all

help:
	@echo "Makefile for $(PROGRAM)"
	@echo ""
	@echo "Available tasks:"
	@echo "all       - do all compilation tasks, create app in build directory"
	@echo "clean     - remove all build artifacts"
	@echo "release   - create a release of the executable in the build/ dir"
	@echo "disk      - generate platform specific disk images in dist/ dir"
	@echo "test      - run application in emulator for given platform."
	@echo "            specific platforms may expose additional variables to run with"
	@echo "            different emulators, see makefiles/custom-<platform>.mk"
	