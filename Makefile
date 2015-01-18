.PHONY: all clean install

all:
	$(MAKE) -C kernel all
	$(MAKE) -C java all
	$(MAKE) -C python all

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C java clean
	$(MAKE) -C python clean

install:
	$(MAKE) -C kernel install
	$(MAKE) -C java install
	$(MAKE) -C python install
