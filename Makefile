CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread
OBJDIR = build

SRC = src/main.c src/registration.c src/doctor.c src/patient.o src/director.c
OBJ = $(SRC:.c=.o)

TARGET = przychodnia

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)
