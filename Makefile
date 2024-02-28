args?='-O2'

filexplr: filexplr.c
	gcc -Wall -Wextra -pedantic $(args) -o fileman fileman.c

clean:
	if [ -f fileman ]; then rm fileman; fi
