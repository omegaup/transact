CXX := g++
CXXFLAGS += -std=c++11 -fPIC -O2 -nodefaultlibs -fno-rtti -fno-exceptions
LDFLAGS += -static-libgcc -static-libstdc++ -lc

PREFIX := /usr

.PHONY: all
all: libtransact.so libtransact.a

libtransact.o: libtransact.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

libtransact.so: libtransact.o
	$(CXX) $(CXXFLAGS) -shared $^ -o $@ $(LDFLAGS)

libtransact.a: libtransact.o
	ar rvs $@ $^

.PHONY: clean
clean:
	rm -f libtransact.so libtransact.o libtransact.a

.PHONY: install
install: libtransact.so libtransact.a
	install -m 0644 libtransact.so $(PREFIX)/lib/x86_64-linux-gnu/
	install -m 0644 libtransact.a $(PREFIX)/lib/x86_64-linux-gnu/
	install -m 0644 libtransact.h $(PREFIX)/include/
