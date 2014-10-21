PROGRAM=ppc64-sim
CC=gcc
TARGET_DIR=obj

$(PROGRAM): main.c main.h file.c file.h
	$(CC) -o $(TARGET_DIR)/$(PROGRAM) main.c file.c
