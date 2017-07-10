
all: clean parse

parse: 
	gcc -Wall nsh.c parse.c -o nsh
clean:
	rm -f nsh
