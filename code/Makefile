ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -o ./build/runner -O2

bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/runner -O2

student:
	@echo " Compile main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/main.c ./src/hash_file.c -lbf -o ./build/runner -O2