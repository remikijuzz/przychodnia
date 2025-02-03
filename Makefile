CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread

SRC_DIR = src

EXECUTABLES = $(SRC_DIR)/przychodnia $(SRC_DIR)/registration $(SRC_DIR)/doctor $(SRC_DIR)/patient

all: $(EXECUTABLES)

$(SRC_DIR)/przychodnia: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -o $@ $<

$(SRC_DIR)/registration: $(SRC_DIR)/registration.c
	$(CC) $(CFLAGS) -o $@ $<

$(SRC_DIR)/doctor: $(SRC_DIR)/doctor.c
	$(CC) $(CFLAGS) -o $@ $<

$(SRC_DIR)/patient: $(SRC_DIR)/patient.c
	$(CC) $(CFLAGS) -o $@ $<

$(SRC_DIR)/director: $(SRC_DIR)/director.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(EXECUTABLES)
