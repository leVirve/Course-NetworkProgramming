all:
	gcc server.c -o server
	gcc client.c -o client

clean:
	rm -rf ./Upload
	rm client
	rm server
