JAVA_HOME := $(shell dirname $(shell dirname $(shell readlink -f $(shell which javac))))
JAVA_INSTALL ?= $(JAVA_HOME)

.PHONY: all
all: bin/libtransact.jar bin/libtransact_java.so

bin:
	mkdir bin

classes:
	mkdir classes

classes/com/omegaup/transact/%.class: src/java/com/omegaup/transact/Message.java \
		src/java/com/omegaup/transact/Interface.java | classes
	javac $^ -d classes

src/c/com_omegaup_transact_Message.h: classes/com/omegaup/transact/Message.class
	javah -o $@ -classpath classes -force com.omegaup.transact.Message

src/c/com_omegaup_transact_Interface.h: classes/com/omegaup/transact/Interface.class
	javah -o $@ -classpath classes -force com.omegaup.transact.Interface

bin/libtransact.jar: classes/com/omegaup/transact/Message.class classes/com/omegaup/transact/Interface.class \
		| bin
	jar cf $@ -C classes .

bin/libtransact_java.so: src/c/com_omegaup_transact_Interface.c src/c/com_omegaup_transact_Message.c \
		| bin src/c/com_omegaup_transact_Message.h src/c/com_omegaup_transact_Interface.h
	gcc -Wl,-soname,libtransact_java.so -Wall -Werror -O2 -g -I$(JAVA_HOME)/include \
		-I$(JAVA_HOME)/include/linux -shared -o $@ $^ -ltransact -fPIC

.PHONY: clean
clean:
	rm -rf bin classes src/c/com_omegaup_transact_Message.h src/c/com_omegaup_transact_Interface.h
	rm $(JAVA_INSTALL)/jre/lib/ext/libtransact*

.PHONY: install
install: bin/libtransact.jar bin/libtransact_java.so
	cp $^ $(JAVA_INSTALL)/jre/lib/ext
