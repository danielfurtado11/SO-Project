all: server client

server: bin/monitor

client: bin/tracer

LIBS = `pkg-config --cflags --libs glib-2.0`

folders:
	@mkdir -p src obj bin tmp PIDS-folder

obj/hashtable.o: src/hashtable.c | folders
	gcc -Wall -g -c src/hashtable.c -o obj/hashtable.o $(LIBS) -lbsd

obj/process.o: src/process.c | folders
	gcc -Wall -g -c src/process.c -o obj/process.o -lbsd

obj/server_funcs.o: src/server_funcs.c | folders
	gcc -Wall -g -c src/server_funcs.c -o obj/server_funcs.o $(LIBS) -lbsd	

obj/monitor.o: src/monitor.c | folders
	gcc -Wall -g -c src/monitor.c -o obj/monitor.o $(LIBS) -lbsd

obj/tracer.o: src/tracer.c | folders
	gcc -Wall -g -c src/tracer.c -o obj/tracer.o -lbsd

bin/monitor: obj/monitor.o obj/hashtable.o obj/process.o obj/server_funcs.o | folders
	gcc -g obj/monitor.o obj/hashtable.o obj/process.o obj/server_funcs.o -o bin/monitor $(LIBS) -lbsd

bin/tracer: obj/tracer.o obj/process.o | folders
	gcc -g obj/tracer.o obj/process.o -o bin/tracer -lbsd

clean:
	rm -f obj/* tmp/* bin/{tracer,monitor}
