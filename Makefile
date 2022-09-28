run-raw-nuova: raw-nuova
	./raw-nuova prog

raw-nuova:
	gcc raw-nuova.c -O3 -o raw-nuova

run-nuova: nuova
	./nuova prog 2> nuova.log

nuova: nuova.c
	gcc nuova.c -g -o nuova

run-test: test
	./test

test: test.c
	gcc test.c -O3 -o test
