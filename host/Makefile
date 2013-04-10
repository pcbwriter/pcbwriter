CXX=gcc -Wall -std=gnu99
LIBS=-lusb-1.0 -lm

.PHONY: all clean

all: read_debug set_speed put_data

read_debug: read_debug.o Makefile
	$(CXX) -o $@ read_debug.o $(LIBS)

set_speed: set_speed.o Makefile
	$(CXX) -o $@ set_speed.o $(LIBS)

put_data: put_data.o Makefile
	$(CXX) -lm -o $@ put_data.o $(LIBS)

%.o: %.c $(HEADERS) Makefile
	$(CXX) -c $<

clean:
	-/bin/rm -f *.o test
