# all after symbol '#' is comment

# === which communication library to use ===
CC	=	c++      # for ethernet and infiniband networks

CFLAGS	=
LIBS	=

default: sada

sada: p1.o

p1.o: ; $(CC) $(CFLAGS) test.cpp ../stack.cpp -o test.o $(LIBS)

clean: ; rm test.o
