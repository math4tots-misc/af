.PHONY: clean

# Should work on MacOS and Linux - clang is reliably aliased to gcc on MacOS
# TODO: Rewrite as a Python script that will work on Windows too

af: $(wildcard src/af/*.hh) $(wildcard src/*.cc)
	g++ -std=c++17 -Wall -Werror -Wpedantic -Isrc src/*.cc -g -o af


clean:
	rm -rf af
