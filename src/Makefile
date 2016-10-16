all: nettail-client nettail-server

nettail-client: nettail-client.c
		gcc -Wall -I ./include nettail-client.c -o nettail-client

nettail-server: nettail-server.c functions.c
		gcc -Wall -std=gnu99 -pthread -I./include -o nettail-server functions.c nettail-server.c


clean:
	rm -f *.o a.out core nettail-client nettail-server 
