all:
	gcc -mmmx -msse2 -msse4.1 -o CHMk-1-2048 CHMk-1-2048.c gost3411-2012-core.c -lcrypto -lssl -lm

test: all
	perl test-CHMk-1-2048.pl
