CC = gcc
CFLAGS = -Wall -g -Iinclude

SRC = src
TARGETS = przychodnia doctor patient registration director

all: $(TARGETS)

przychodnia: $(SRC)/main.c
	$(CC) $(CFLAGS) $< -o $@

doctor: $(SRC)/doctor.c
	$(CC) $(CFLAGS) $< -o $@

patient: $(SRC)/patient.c
	$(CC) $(CFLAGS) $< -o $@

registration: $(SRC)/registration.c
	$(CC) $(CFLAGS) $< -o $@

director: $(SRC)/director.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS) *.txt report.txt

.PHONY: all clean
