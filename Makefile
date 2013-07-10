PKGS = glib-2.0

CFLAGS += $(shell pkg-config --cflags $(PKGS))
CFLAGS += -std=c99 -O3 -Wall -Werror

LDFLAGS += $(shell pkg-config --libs $(PKGS))
LDFLAGS += -lOpenCL

BIN = check
OBJS = check.o ocl.o


.PHONY: all clean


$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

all: $(BIN)

clean:
	rm -f $(BIN) $(OBJS)
