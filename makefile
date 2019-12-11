all:
	gcc -O server.c -o server -levent
	gcc -O client.c -o client -levent

clean:
	rm server client 
