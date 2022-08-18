SRC	:= elf_size.c
ELF	:= size

CFLAGS	:= -Werror -Wall -I inc/

default: $(ELF)

.PHONY: clean $(ELF)
$(ELF): $(SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o size
