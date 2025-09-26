.PHONY: example
example: example/setup.c corm_setup.c example/example.c
	gcc -ggdb -o example/setup example/setup.c corm_setup.c
	./example/setup
	gcc -ggdb -o example/example example/example.c example/corm_user.c -L./Jacon -ljacon

clean:
	rm -f example/setup example/example example/corm_user.c example/corm_user.h