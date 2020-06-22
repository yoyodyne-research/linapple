CXX ?= c++
CXXFLAGS ?= -g -O3
CXXFLAGS += -w

OBJS := $(wildcard *.cpp)
OBJS := $(OBJS:.cpp=.o)

SDL_CONFIG ?= sdl-config
SDL_CFLAGS = $(shell $(SDL_CONFIG) --cflags)
SDL_LIBS = $(shell $(SDL_CONFIG) --libs)

CURL_CONFIG ?= curl-config
CURL_CFLAGS = $(shell $(CURL_CONFIG) --cflags)
CURL_LIBS = $(shell $(CURL_CONFIG) --libs)

CXXFLAGS += $(SDL_CFLAGS)
CXXFLAGS += $(CURL_CFLAGS)
LDFLAGS += $(SDL_LIBS) $(CURL_LIBS) -lSDL_image -lz -lzip

TARGET = gala

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	- rm $(TARGET) *.o

install: all
	install -D --target-directory $(DESTDIR)/bin $(TARGET)
	install -D --target-directory $(DESTDIR)/share/gala ../startup.dsk 
	install -D --target-directory $(DESTDIR)/etc/gala ../gala.conf
