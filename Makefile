CC 		:= gcc
CFLAGS 	:= -Wall -Wextra -ffunction-sections -fdata-sections
LFLAGS	:= -Wl,--gc-sections -Wl,-s

BUILD_DIR := build
LIB_NAME := libcorm

$(BUILD_DIR):
	mkdir -p $@

jacon.o: src/jacon.c src/jacon.h
	$(CC) $(CFLAGS) -c src/jacon.c -o $(BUILD_DIR)/jacon.o

corm.o: src/corm.c src/corm.h jacon.o
	$(CC) $(CFLAGS) -c src/corm.c -o $(BUILD_DIR)/corm.o

static: $(BUILD_DIR) corm.o jacon.o
	ar rcs $(BUILD_DIR)/$(LIB_NAME).a $(BUILD_DIR)/corm.o $(BUILD_DIR)/jacon.o

shared: $(BUILD_DIR) corm.o jacon.o
	$(CC) -fPIC -shared -o $(BUILD_DIR)/$(LIB_NAME).so $(BUILD_DIR)/corm.o $(BUILD_DIR)/jacon.o

jsonex: $(BUILD_DIR) 
	$(CC) $(LFLAGS) -o $(BUILD_DIR)/jsonsetup examples/json/setup.c src/corm_setup*.c
	./$(BUILD_DIR)/jsonsetup
	$(CC) $(LFLAGS) -Isrc -Ibuild -o $(BUILD_DIR)/jsonexample examples/json/example.c \
		$(BUILD_DIR)/corm_user.c src/corm.c src/jacon.c

clean:
	rm -rf $(BUILD_DIR)