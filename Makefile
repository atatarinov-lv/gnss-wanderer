LIBRARY_NAME := gnss-wanderer

CFLAGS = -g -O2 -Wall -Wextra -Isrc -rdynamic -DNDEBUG $(OPTFLAGS)
LIBS = -ldl $(OPTLIBS)
PREFIX ?= /usr/local

MAIN=src/main.c

SOURCES = $(filter-out $(MAIN), $(wildcard src/**/*c src/*.c))
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

TEST_SRC = $(wildcard tests/*_tests.c)
TESTS = $(patsubst %.c,%,$(TEST_SRC))

TARGET = build/lib$(LIBRARY_NAME).a
SO_TARGET = $(patsubst %.a,%.so,$(TARGET))
BIN_TARGET = bin/$(LIBRARY_NAME)

# The Target Build
all: $(TARGET) $(SO_TARGET) tests $(BIN_TARGET)

dev: CFLAGS = -g -Wall -Wextra -Isrc $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@
$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)
$(BIN_TARGET): $(SO_TARGET)
	$(CC) $(CFLAGS) -o $@ $(MAIN) $(TARGET)

build:
	@mkdir -p build
	@mkdir -p bin

# The Unit Tests
.PHONY: tests
tests:
	for i in $(TESTS) ; do \
		$(CC) $(CFLAGS) -o $$i $$i.c $(TARGET) ;\
	done
	sh ./tests/runtests.sh

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/

# The Checker
check:
	@echo Files with potentially dangerous functions:
	@grep -E '[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' $(SOURCES) || echo "None found"
