all: myls

myls: clean myls.o
	gcc myls.o -o myls

myls.o:
	gcc -c myls.c
clean:
	rm *.o myls
