all: sender reciever

sender: ./sender/dns_receiver_events.o ./sender/dns_receiver_events.h

reciever: ./sender/dns_sender_events.o ./sender/dns_sender_events.h

clean:
	rm *.o dns_receiver_events