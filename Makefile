
.PHONY: default clean install

default:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

install:
	$(MAKE) -C install

