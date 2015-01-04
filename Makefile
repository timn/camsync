
PARALLEL_JOBS = -j2

# The all target does the heavy lifting, creating the build directory and
# invoking CMake
all:
	@mkdir -p build
	-mkdir -p bin
	cd build && cmake $(CMAKE_FLAGS) ..
ifneq ($(MAKE),)
	cd build && $(MAKE) $(PARALLEL_JOBS)
else
	cd build && make $(PARALLEL_JOBS)
endif

# The clean target blows everything away
# It also removes auto-generated message/service code directories, 
# to handle the case where the original .msg/.srv file has been removed,
# and thus CMake no longer knows about it.
clean:
	-cd build && make clean
	rm -rf build

