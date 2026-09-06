CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Os -pipe
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -D_FILE_OFFSET_BITS=64
CXXFLAGS += -Isrc

LDFLAGS  ?=

# FLTK via pkg-config (works on most distros)
FLTK_CFLAGS  := $(shell pkg-config --cflags fltk 2>/dev/null)
FLTK_LIBS    := $(shell pkg-config --libs fltk 2>/dev/null || echo "-lfltk -lfltk_images")
CXXFLAGS     += $(FLTK_CFLAGS)

LIBS     := $(FLTK_LIBS)
LIBS     += -lcurl -lpthread

# Uncomment for 32-bit cross-compile (e.g. on x86_64 targeting i686):
# CXXFLAGS += -m32
# LDFLAGS  += -m32

# Uncomment for fully static build (max portability, Void/musl/etc):
# LDFLAGS += -static
# LIBS    := /usr/lib/libfltk.a /usr/lib/libcurl.a /usr/lib/libssl.a /usr/lib/libcrypto.a -lpthread -lz

PREFIX   ?= /usr/local
DESTDIR  ?=

SRCS := $(wildcard src/core/*.cpp src/launcher/*.cpp src/ui/*.cpp src/*.cpp) vendor/cJSON.c
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRCS)))
BIN  := lightcraft

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

vendor/%.o: vendor/%.c
	$(CC) $(CXXFLAGS) -Ivendor -w -c -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)

install: $(BIN)
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
