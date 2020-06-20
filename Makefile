
.PHONY: default clean install

default:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

format: src/cli.cpp src/cli.h src/main.h src/main.cpp
	clang-format -i $^

install:
	$(MAKE) -C install

