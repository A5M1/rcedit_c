CC = x86_64-w64-mingw32-gcc
CFLAGS = -O2 -Wall -Wextra -municode -DUNICODE -D_UNICODE
WINDRES = x86_64-w64-mingw32-windres
TARGET = test_prog.exe
SRC = test_prog.c
RC = test_prog.rc
RESOBJ = test_prog_res.o

all: build_test_prog

build_test_prog: $(SRC) $(RC)
	$(WINDRES) --input-format=rc --output-format=coff --target=pe-x86-64 $(RC) -o $(RESOBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(RESOBJ)

clean:
	-rm -f $(TARGET) $(RESOBJ)

.PHONY: all clean build_test_prog
