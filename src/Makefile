PREFIX = /usr/local
CFLAGS ?= -Wall -Wextra -O0 -g `dnet-config --cflags`
LIBS = -lpthread `dnet-config --libs` -lzmq -lircclient -lcrypto -lm -lbsd -lpcre -lconfuse
OBJS = ipoirc.o irc.o irc_events.o irc_helpers.o ltun.o tun.o tun_helpers.o helpers.o base64.o
TARGET = ipoirc

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

tests:
	$(CC) -o b64test -DTESTS -Wall base64.c -lm -lcrypto

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFINES) -o $@ -c $<

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	install -p -m 755 $(TARGET) $(PREFIX)/bin

clean:
	rm -f $(OBJS) $(TARGET) b64test *~

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

run: $(TARGET)
	./$(TARGET) $@

