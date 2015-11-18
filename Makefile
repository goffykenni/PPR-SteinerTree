# all after symbol '#' is comment

# === which communication library to use ===
CC	=	mpiCC      # for ethernet and infiniband networks

CFLAGS	=
LIBS	=	-lm

default: sada

sada: p1.o

p1.o: ; $(CC) $(CFLAGS) main.cpp checker.cpp unionfind.cpp stack.cpp graph.cpp -o steiner $(LIBS)

clean: ; rm p?\.o

