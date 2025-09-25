main: main.c
	gcc -ggdb -o main main.c -L./Jacon -ljacon

corm: corm.c
	gcc -ggdb -o corm corm.c

corm.o: corm.c corm.h

.PHONY: example
example: example/corm_setup.c
	gcc -ggdb -o example/corm_setup example/corm_setup.c corm.c
	./example/corm_setup
	gcc -ggdb -o example/example example/example.c example/corm_generated.c -L./Jacon -ljacon

clean:
	rm -f corm main
	rm -f example/corm_setup example/example example/corm_generated.c example/corm_generated.h