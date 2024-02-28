args?='-O2'

filexplr: filexplr.c
	gcc -Wall -Wextra -pedantic $(args) -o filexplr filexplr.c

clean:
	if [ -f filexplr ]; then rm filexplr; fi
