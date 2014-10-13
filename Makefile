PROGRAM=ppc64-sim
CC=gcc
TARGET_DIR=obj

$(PROGRAM): main.c main.h
	$(CC) -o $(TARGET_DIR)/$(PROGRAM) main.c
