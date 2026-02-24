LIBRARY_NAME := gnss-wanderer
CDMFTM := cmdfmt

LIBS = -lm
DEFINES = -D_UBLOX8

PREFIX ?= /usr/local

M_CFLAGS = -g -Wall -Wextra -Isrc $(DEFINES)

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	LIBS += -ludev -latomic
	DEFINES += -D_LINUX
else ifeq ($(UNAME_S),Darwin)
	LIBS += -latomic_ops
	DEFINES += -D_MACOS
	M_CFLAGS += -L$(shell brew --prefix)/lib -framework IOKit -framework CoreFoundation
else
	$(error Unsupported OS: $(UNAME_S))
endif

CFLAGS = $(M_CFLAGS) -O2 -rdynamic -DNDEBUGVV $(OPTFLAGS)

MAIN=cmd/main.c

SOURCES = $(wildcard src/**/*c src/*.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

TEST_SRC = $(wildcard tests/*_tests.c)
TESTS = $(patsubst %.c,%,$(TEST_SRC))

TARGET = build/lib$(LIBRARY_NAME).a
SO_TARGET = $(patsubst %.a,%.so,$(TARGET))
BIN_TARGET = bin/$(LIBRARY_NAME)

# The Target Build
all: $(TARGET) $(SO_TARGET) $(BIN_TARGET) $(CDMFTM)

dev: CFLAGS = $(M_CFLAGS) $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@
$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

$(BIN_TARGET): CFLAGS = $(M_CFLAGS) $(OPTFLAGS)
$(BIN_TARGET): $(SO_TARGET)
	$(CC) $(CFLAGS) -o $@ $(MAIN) $(TARGET) $(LIBS)

$(CDMFTM): CFLAGS = $(M_CFLAGS) $(OPTFLAGS)
$(CDMFTM): $(TARGET)
	$(CC) $(CFLAGS) -o bin/$@ cmd/$@.c $(TARGET) $(LIBS)

build:
	@mkdir -p build
	@mkdir -p bin

# The Unit Tests
.PHONY: build_tests
build_tests: CFLAGS = $(M_CFLAGS) $(OPTFLAGS)
build_tests: $(TARGET)
	for i in $(TESTS) ; do \
		$(CC) $(CFLAGS) -o $$i $$i.c $(TARGET) $(LIBS);\
	done

.PHONY: tests
tests: build_tests
	sh ./tests/runtests.sh

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS) bin/*
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

cc-info:
	@echo "CC: " $(CC)
