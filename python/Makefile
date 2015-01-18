TARGET := build/lib.linux-x86_64-2.7/transact.so

.PHONY: all clean install

all: $(TARGET)

clean:
	rm -rf build

$(TARGET): transactmodule.c transactmodule.h
	python setup.py build

install:
	python setup.py install
