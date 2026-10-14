all: prog1
prog1: prog1.c
	gcc -Wall -fsanitize=address,undefined -o prog1 prog1.c

prog2: prog2.c
	gcc -Wall -fsanitize=address,undefined -o prog2 prog2.c

prog3: prog3.c
	gcc -Wall -fsanitize=address,undefined -o prog3 prog3.c

lab1: lab1.c
	gcc -Wall -fsanitize=address,undefined -o lab1 lab1.c
.PHONY: clean all
clean:
	rm prog1 prog2 prog3 lab1
