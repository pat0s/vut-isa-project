#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "dns_receiver_events.h"

#define PORT 53
#define MAX_SIZE 1024

#define NETADDR_STRLEN (INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN)
#define CREATE_IPV4STR(dst, src) char dst[NETADDR_STRLEN]; inet_ntop(AF_INET, src, dst, NETADDR_STRLEN)
#define CREATE_IPV6STR(dst, src) char dst[NETADDR_STRLEN]; inet_ntop(AF_INET6, src, dst, NETADDR_STRLEN)

void dns_receiver__on_query_parsed(char *filePath, char *encodedData)
{
	fprintf(stderr, "[PARS] %s '%s'\n", filePath, encodedData);
}

void on_chunk_received(char *source, char *filePath, int chunkId, int chunkSize)
{
	fprintf(stderr, "[RECV] %s %9d %dB from %s\n", filePath, chunkId, chunkSize, source);
}

void dns_receiver__on_chunk_received(struct in_addr *source, char *filePath, int chunkId, int chunkSize)
{
	CREATE_IPV4STR(address, source);
	on_chunk_received(address, filePath, chunkId, chunkSize);
}

void dns_receiver__on_chunk_received6(struct in6_addr *source, char *filePath, int chunkId, int chunkSize)
{
	CREATE_IPV6STR(address, source);
	on_chunk_received(address, filePath, chunkId, chunkSize);
}

void on_transfer_init(char *source)
{
	fprintf(stderr, "[INIT] %s\n", source);
}

void dns_receiver__on_transfer_init(struct in_addr *source)
{
	CREATE_IPV4STR(address, source);
	on_transfer_init(address);
}

void dns_receiver__on_transfer_init6(struct in6_addr *source)
{
	CREATE_IPV6STR(address, source);
	on_transfer_init(address);
}

void dns_receiver__on_transfer_completed(char *filePath, int fileSize)
{
	fprintf(stderr, "[CMPL] %s of %dB\n", filePath, fileSize);
}

int main(int argc, char* argv[])
{
	// 2 mandatory arguments
	if (argc != 3)
	{
		fprintf(stderr, "Wrong number of arguments");
		return 1;
	}

	// TODO: podpora aj IPv6

	// create socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
	{
		fprintf(stderr, "ERROR: Cannot open new socket!\n");
		exit(1);
	}

	struct sockaddr_in socketAddr;
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_addr.s_addr = INADDR_ANY;
  	socketAddr.sin_port = htons(PORT);

	// bind port
	if (bind(sockfd, (struct sockaddr *)&sockfd, sizeof(sockfd)) < 0)
	{
		fprintf(stderr, "ERROR: Cannot bind socket!\n");
		exit(1);
	}

	while(1)
	{
		// recvfrom();
		// sendto();
	}

	return 0;
}
