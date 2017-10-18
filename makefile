#CFLAGS = -O3 -Wall
CFLAGS = -Wall -g
MODES = netHog slowboat noBoat socketListenTest

all: $(MODES)

%.o: %.c %.h
	gcc $(CFLAGS) -c -o $@ $< -lcurses

netHog: netHog.o curseNetHog.o
	gcc $(CFLAGS) -o netHog netHog.o curseNetHog.o -lcurses

slowboat: slowboat.o
	gcc $(CFLAGS) -o slowboat slowboat.o

noBoat: noBoat.o
	gcc $(CFLAGS) -o noBoat noBoat.o

socketListenTest: socketListenTest.o
	gcc $(CFLAGS) -o socketListenTest socketListenTest.o

clean:
	rm *.o $(MODES)
