CC = gcc
CFLAGS = -Wall
OBJFILES_SEND = ./sender/dns_sender_events.o base32.o
OBJFILES_RECV = ./receiver/dns_receiver_events.o base32.o
TARGET_SEND = dns_sender
TARGET_RECV = dns_receiver


all: $(TARGET_SEND) $(TARGET_RECV)
sender: $(TARGET_SEND)
receiver: $(TARGET_RECV)

$(TARGET_SEND): $(OBJFILES_SEND)
	$(CC) $(CFLAGS) -o $(TARGET_SEND) $(OBJFILES_SEND)

$(TARGET_RECV): $(OBJFILES_RECV)
	$(CC) $(CFLAGS) -o $(TARGET_RECV) $(OBJFILES_RECV)

clean:
	rm -f $(OBJFILES_SEND) $(OBJFILES_RECV) $(TARGET_SEND) $(TARGET_RECV) *~
