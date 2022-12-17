CC=gcc
OBJS= main.o smallsh.o

all: main.out

main.out : $(OBJS)
	$(CC) -o $@ $(OBJS)

main.o: main.c smallsh.h
	$(CC) -c -o $@ $<

smallsh.o: smallsh.c smallsh.h
	$(CC) -c -o $@ $<


clean:
	rm -f *.o main.out main smallsh
