all: sender receiver

receiver: ./receiver/dns_receiver_events.c ./receiver/dns_receiver_events.h
	gcc ./receiver/dns_receiver_events.c -o dnsreceiver

clean:
	rm ./sender/*.o ./receiver/*.o  ./sender/server ./receiver/receiver
