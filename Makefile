run-raw-nuova: bin/raw-nuova bin/prog
	./bin/raw-nuova bin/prog

bin/raw-nuova:
	gcc raw-nuova.c -O3 -o bin/raw-nuova

run-nuova: bin/nuova bin/prog
	./bin/nuova bin/prog 2> logs/nuova.log

bin/nuova: nuova.c
	gcc nuova.c -g -o bin/nuova

run-compiler: bin/compiler
	cat sources/$(prog).s \
		| awk '{gsub(";", ";\\"); print}' \
		> cache/$(prog).c
	gcc -E cache/$(prog).c \
		| grep -v "#" \
		| awk '{gsub(";", "\n "); print}' \
		> cache/$(prog).s
	./bin/compiler cache/$(prog).s 2> logs/compiler.log

bin/compiler: compiler.c
	gcc compiler.c -g -O3 -o bin/compiler
