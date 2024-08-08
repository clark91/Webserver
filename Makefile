main: main.c parser.c
	gcc -o main main.c -lws2_32 parser.c -I.
