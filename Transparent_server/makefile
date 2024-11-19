all:server

server:server.o
	gcc $^ -o $@ -lsqlite3

%.o:%.c
	gcc -c $^ -o $@

clean:
	rm *.o server