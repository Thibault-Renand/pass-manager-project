ASM = nasm
ASMFLAGS = -f elf64
LD = ld
TARGET = secure_vault
SRC = secure_vault.asm
OBJ = secure_vault.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) -o $@ $(OBJ)

$(OBJ): $(SRC) inc/cours.inc
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) vault.db

.PHONY: all clean
