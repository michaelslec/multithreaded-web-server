httpserver: httpserver.o queue.o
	gcc -Wall -Wextra -Wpedantic -Wshadow -pthread -o httpserver httpserver.o queue.o

httpserver.o: httpserver.c
	gcc -Wall -Wextra -Wpedantic -Wshadow -pthread -c httpserver.c
    
queue.o: queue.c
	gcc -Wall -Wextra -Wpedantic -Wshadow -pthread -c queue.c
    
clean:
	rm -f httpserver
