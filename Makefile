CC=	 	gcc
INC=	-I/usr/include/python2.7
EXEC=	main

all: 	main

main:	main.c
	$(CC) $(INC) -Wall main.c -lpython2.7 -o $(EXEC)

debug:	$(CC) $(INC) -g main.c -lpython2.7 -o $(EXEC)

clean:	
	rm $(EXEC)