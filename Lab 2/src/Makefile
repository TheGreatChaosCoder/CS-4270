mu-riscv: mu-riscv.c mu-assem.cpp
	gcc -Wall -g -O2 $< -o $@
	g++ -Wall -g -ggdb -O2 mu-assem.cpp -o mu-assem

.PHONY: clean
clean:
	rm -rf *.o *~ mu-riscv
