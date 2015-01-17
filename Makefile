.PHONY: all clean install

all:
	$(MAKE) -C kernel all
	$(MAKE) -C java all

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C java clean

install:
	$(MAKE) -C kernel install
	$(MAKE) -C java install
