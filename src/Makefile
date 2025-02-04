CC = gcc
CFLAGS = -Wall -g

all: przychodnia doctor patient registration

przychodnia: main.c
	$(CC) $(CFLAGS) main.c -o przychodnia

doctor: doctor.c
	$(CC) $(CFLAGS) doctor.c -o doctor

patient: patient.c
	$(CC) $(CFLAGS) patient.c -o patient

registration: registration.c
	$(CC) $(CFLAGS) registration.c -o registration

clean:
	rm -f przychodnia doctor patient registration
