all: stn hub

tokRing.o: tokRing.c
	cc -c tokRing.c

stn: stn.c tokRing.o
	cc -o stn stn.c tokRing.o

hub: hub.c
	cc -o hub hub.c -lpthread
