#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "dns_sender_events.h"

#define NETADDR_STRLEN (INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN)
#define CREATE_IPV4STR(dst, src) char dst[NETADDR_STRLEN]; inet_ntop(AF_INET, src, dst, NETADDR_STRLEN)
#define CREATE_IPV6STR(dst, src) char dst[NETADDR_STRLEN]; inet_ntop(AF_INET6, src, dst, NETADDR_STRLEN)


void dns_sender__on_chunk_encoded(char *filePath, int chunkId, char *encodedData)
{
	fprintf(stderr, "[ENCD] %s %9d '%s'\n", filePath, chunkId, encodedData);
}

void on_chunk_sent(char *source, char *filePath, int chunkId, int chunkSize)
{
	fprintf(stderr, "[SENT] %s %9d %dB to %s\n", filePath, chunkId, chunkSize, source);
}

void dns_sender__on_chunk_sent(struct in_addr *dest, char *filePath, int chunkId, int chunkSize)
{
	CREATE_IPV4STR(address, dest);
	on_chunk_sent(address, filePath, chunkId, chunkSize);
}

void dns_sender__on_chunk_sent6(struct in6_addr *dest, char *filePath, int chunkId, int chunkSize)
{
	CREATE_IPV6STR(address, dest);
	on_chunk_sent(address, filePath, chunkId, chunkSize);
}

void on_transfer_init(char *source)
{
	fprintf(stderr, "[INIT] %s\n", source);
}

void dns_sender__on_transfer_init(struct in_addr *dest)
{
	CREATE_IPV4STR(address, dest);
	on_transfer_init(address);
}

void dns_sender__on_transfer_init6(struct in6_addr *dest)
{
	CREATE_IPV6STR(address, dest);
	on_transfer_init(address);
}

void dns_sender__on_transfer_completed( char *filePath, int fileSize)
{
	fprintf(stderr, "[CMPL] %s of %dB\n", filePath, fileSize);
}

#define DNS_PORT 53
#define MAX_DNS_SERVER_IP 100

int getSystemDnsServer(char *dnsServer)
{
	FILE *file;

	file = popen("cat /etc/resolv.conf | grep nameserver | head -1 | awk -F' ' '{print $2}'" , "r");
	if (file == NULL) exit(1);

	fgets(dnsServer, MAX_DNS_SERVER_IP, file);
    // remove /n from fgets
    dnsServer[strcspn(dnsServer, "\r\n")] = 0;
	pclose(file);
		
	return 0;
}


int main(int argc, char* argv[])
{
	char dnsServer[MAX_DNS_SERVER_IP+1];

	if (getSystemDnsServer(dnsServer) == 1)
	{
		// TODO: error message
		exit(1);
	}

	// create socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
	{
		// TODO: error message
		exit(1);
	}

	// set socket option
	const int enabled = 1;
	int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
	if (err < 0)
	{
		fprintf(stderr, "ERROR: Cannot set options on socket\n");
		exit(1);
	}

	// struct sockaddr_in socketAddr;
	// socketAddr.sin_addr = // *ipAddr;
	// socketAddr.sin_family = AF_INET;
	// socketAddr.sin_port = htons(DNS_PORT);

	// int err = sendto(sockfd, , , MSG_CONFIRM, (struct sockaddr *)&socketAddr, sizeof(socketAddr) );
	// if (err < 0)
	// {
	// 	fprintf(stderr, "Error: sendto\n");
	// 	exit(1);
	// }

  	// if (sendto(sockfd, dns_buf, buf_size, MSG_CONFIRM, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in)) == -1)

  	// unsigned char response[1024];
  	// int num_received;
  	// socklen_t socklen = sizeof(struct sockaddr_in);
  	// if ((num_received = recvfrom(sockfd, response, sizeof(response), MSG_WAITALL, (struct sockaddr *)&sockaddr, &socklen)) == -1) {

	return 0;
}