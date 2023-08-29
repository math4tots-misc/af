.PHONY: clean

# Should work on MacOS and Linux - clang is reliably aliased to gcc on MacOS
# TODO: Rewrite as a Python script that will work on Windows too


WARNING_FLAGS := -Wall -Werror -Wpedantic

# These flags might break on linux/gcc
CLANG_WARNING_FLAGS := -Weverything \
	-Wno-poison-system-directories \
	-Wno-c++98-compat \
	-Wno-padded \
	-Wno-documentation \
	-Wno-unused-parameter

FLAGS := -std=c++17 $(WARNING_FLAGS) $(CLANG_WARNING_FLAGS) -Isrc -g

af: $(wildcard src/af/*.hh) $(wildcard src/*.cc)
	g++ $(FLAGS) src/*.cc -o af


clean:
	rm -rf af
