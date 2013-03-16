CXX=gcc -Wall -std=gnu99
LIBS=-lusb-1.0

.PHONY: all clean

all: read_debug set_speed put_data

read_debug: read_debug.o Makefile
	$(CXX) $(LIBS) -o $@ read_debug.o

set_speed: set_speed.o Makefile
	$(CXX) $(LIBS) -o $@ set_speed.o

put_data: put_data.o Makefile
	$(CXX) $(LIBS) -lm -o $@ put_data.o

%.o: %.c $(HEADERS) Makefile
	$(CXX) -c $<

clean:
	-/bin/rm -f *.o test
