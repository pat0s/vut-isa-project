#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include "dns_receiver_events.h"
#include "../error.h"
#include "../dns.h"
#include "../base32.h"

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


// TODO: vymazat
void print_buffer(unsigned char *buffer, size_t len) {
  unsigned char preview[17];
  preview[16] = '\0';
  memset(preview, ' ', 16);
  for (int i = 0; i < len; ++i) {
    if (i && i % 16 == 0) {
      printf(" %s\n", preview);
      memset(preview, ' ', 16);
    }
    unsigned char c = buffer[i];
    printf("%02x ", c);
    preview[i % 16] = (c == ' ' || (c >= '!' && c < '~')) ? c : '.';
  }
  for (int i = 0; i < 16 - len % 16; ++i) {
    printf("   ");
  }
  printf(" %s\n", preview);
}

void changeHostToDnsFormat(unsigned char* dnsBuffer, unsigned char* host) 
{
	int lock = 0 , i;
	strcat((char*)host,".");
	
	for(i = 0 ; i < strlen((char*)host) ; i++) 
	{
		if(host[i]=='.') 
		{
			*dnsBuffer++ = (unsigned char)(i-lock);
			for(;lock<i;lock++) 
			{
				*dnsBuffer++ = host[lock];
			}
			lock++;
		}
	}
	*dnsBuffer++= (unsigned char)0;
}

int main(int argc, char* argv[])
{
	// check and store parameters
	// 2 mandatory parameters - BASE_PATH, DST_FILEPATH
	if (argc != 3)
	{
		fprintf(stderr, "Wrong number of arguments!\n");
		return 1;
	}

    char *BASE_PATH = argv[1];
	char *DST_FILEPATH = argv[2];

	// TODO: IPv6

	// create socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
	{
		fprintf(stderr, "ERROR: Cannot open new socket!\n");
		exit(CREATE_SOCK_ERR);
	}

	// set socket option
	const int enabled = 1;
	int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
	if (err < 0)
	{
		fprintf(stderr, "ERROR: Cannot set options on socket!\n");
		exit(SOCK_OPT_ERR);
	}

	struct sockaddr_in socketAddr, clientAddr;
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_addr.s_addr = INADDR_ANY;
	socketAddr.sin_port = htons(DNS_PORT);

	// bind port
	if (bind(sockfd, (const struct sockaddr *)&socketAddr, sizeof(socketAddr)) < 0)
	{
		fprintf(stderr, "ERROR: Cannot bind socket!\n");
		exit(BIND_SOCK_ERR);
	}

	unsigned char buffer[MAX_BUFF_SIZE];
	memset(buffer, 0, MAX_BUFF_SIZE);
	int length = sizeof(socketAddr);

	unsigned char hostDnsFormat[253] = {'\0'}; 
	changeHostToDnsFormat(hostDnsFormat, BASE_PATH);

	while(1)
	{
		int n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, MSG_WAITALL, (struct sockaddr *)&socketAddr, (socklen_t *)&length);

		char client_addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), client_addr_str, INET_ADDRSTRLEN);
		
		// TODO: vymazat
		printf("---------------------------\nReceived %d bytes from %s\n", n, client_addr_str);
		print_buffer(buffer, n);
		
		char *dnsQuery = (char *)&buffer[sizeof(struct dns_header)];
		char *ptr = strstr(dnsQuery, (char *)hostDnsFormat);
		int dataLength = 0;
		if (ptr)
		{
			dataLength = (int)(ptr-dnsQuery);
			// printf("%d", dataLength);
		}

		char encodedData[253];
		int start = 1;
		int labelLength = (dataLength >= 63) ? 63 : dataLength;
		// pozicia na ktoru sa to ma kopirovat
		int pos = 0;

		while (start < dataLength)
		{
			labelLength = (dataLength >= start+63) ? 63 : dataLength;
			strncpy(encodedData+pos, dnsQuery+start, labelLength);
			// printf("%s\n", encodedData);
			// dlzka labelu + 1, kvoli cislu ktore urcuje dlzku labelu
			start += labelLength+1;
			pos += labelLength;
		}
		printf("%s\n", encodedData);

		char *decodedData;
		int lengthOfDecodedData = base32_decode((u_int8_t*)encodedData, (u_int8_t*)decodedData, strlen(encodedData));
		printf("%s\n", decodedData);


		// int n = base32_decode();

		// struct dns_header *header = (struct dns_header *)buffer;

		// if (sendto(sockfd, buffer, response_length, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1)
		// {
		// 	fprintf(stderr, "ERROR: Send response to client!\n");
		// 	exit(1);
		// }
    }

	return RECEIVER_OK;
}