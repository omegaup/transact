.PHONY: all clean install

all:
	$(MAKE) -C kernel all

clean:
	$(MAKE) -C kernel clean

install:
	$(MAKE) -C kernel install
