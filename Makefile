CC=gcc
LIBS = -Llib $(patsubst lib%.a,-l%,$(filter lib%.a, $^))

CFLAGS+=-Iinclude
LDFLAGS?=-fPIC


all: bdcap client
bdcap: bdcap.o
	$(CC) $(LDFLAGS) -o $@ $(filter %.o, $^) $(LIBS)
client: client.o
	$(CC) $(LDFLAGS) -o $@ $(filter %.o, $^) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -g -Wall -o $@ -c $<

clean:
	-rm -f bdcap client
	-rm -f *.o

.PHONY: all clean

