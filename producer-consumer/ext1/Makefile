all: prod manager

prod: prod.o
	gcc prod.o -o prod -pthread -lrt
manager: manager.o
	gcc manager.o -o man -pthread -lrt
