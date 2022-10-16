all: sender receiver

receiver: ./receiver/dns_receiver_events.c ./receiver/dns_receiver_events.h error.h dns.h
	gcc ./receiver/dns_receiver_events.c -o dnsreceiver

sender: ./sender/dns_sender_events.c ./sender/dns_sender_events.h error.h dns.h
	gcc ./sender/dns_sender_events.c -o dnssender

clean:
	rm ./sender/*.o ./receiver/*.o  ./sender/server ./receiver/receiver
