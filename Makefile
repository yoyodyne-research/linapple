
.PHONY: clean default distclean doc format install test

default:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

distclean: clean
	- rm -r doc

format: src/cli.cpp src/cli.h src/main.h src/main.cpp
	clang-format -i $^

test: default
	src/gala
	
doc:
	doxygen

install:
	$(MAKE) -C src install

