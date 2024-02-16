args?='-O2'

fileman: fileman.c
	gcc -Wall -Wextra -pedantic $(args) -o fileman fileman.c
