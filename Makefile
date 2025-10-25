CC = gcc
CFLAGS = -Wall -g -I. -lpthread
DEPS =

# Regla genérica para compilar .c a .o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Compila ambos programas
all: cyclictestURJC 

cyclictestURJC: cyclictestURJC.o
	$(CC) -o cyclictestURJC cyclictestURJC.o $(CFLAGS)


clean:
	rm -f *.o cyclictestURJC

