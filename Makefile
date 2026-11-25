CC ?= x86_64-w64-mingw32-gcc
WINDRES ?= x86_64-w64-mingw32-windres
CFLAGS ?= -O2 -Wall -Wextra -municode -DUNICODE -D_UNICODE
LDLIBS ?= -lversion -luser32

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := rcedit.exe

SOURCES := src/main.c src/containers.c src/version.c src/resources.c src/utils.c
OBJECTS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
RESOURCE := $(OBJ_DIR)/rcedit.res.o

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: src/%.c src/rcedit.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(RESOURCE): src/rcedit.rc | $(OBJ_DIR)
	$(WINDRES) --input-format=rc --output-format=coff --target=pe-x86-64 $< -o $@

$(TARGET): $(OBJECTS) $(RESOURCE)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(RESOURCE) $(LDLIBS)

clean:
	-rm -rf $(BUILD_DIR)
	-rm -f $(TARGET)

.PHONY: all clean
