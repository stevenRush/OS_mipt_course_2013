all: ping_base ping1 ping2 ping3

ping_base: ping_base.o
	gcc ping_base.o -o ping_base -pthread -lrt

ping1: ping1.o
	gcc ping1.o -o ping1 -pthread -lrt

ping2: ping2.o
	gcc ping2.o -o ping2 -pthread -lrt

ping3: ping3.o
	gcc ping3.o -o ping3 -pthread -lrt

clean:
	rm *.o
