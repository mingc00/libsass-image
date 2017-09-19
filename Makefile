SOURCES := $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

CFLAGS += -Wall -O2 -fPIC
LDFLAGS += -Wall -Wl,--no-undefined -ldl
LDFLAGS += -Wl,-rpath='$$ORIGIN:/usr/local/lib'
LDLIBS := -lsass

PLUGIN := libsass-image.so

all: $(PLUGIN)

$(PLUGIN): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(OBJECTS) $(PLUGIN)
