all: clean boids

boids:
	mkdir -p bin
	gcc -O3 -Wall -o bin/boids main.c -Iinclude -Llib lib/libraylib.a -lraylib -lm -ldl

clean:
	rm -rf bin/*