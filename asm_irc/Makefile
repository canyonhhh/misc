malus: malus.asm
	nasm -f elf64 malus.asm 
	ld -o main malus.o --strip-all
	rm malus.o

debug:
	nasm -gdwarf -f elf64 malus.asm
	ld -o main malus.o
	rm malus.o
clean:
	rm main
