#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
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

// TODO: delete, only for testing
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
	// check number of parameters
	// 2 mandatory parameters - BASE_PATH, DST_DIRPATH
	if (argc != 3)
	{
		fprintf(stderr, "Wrong number of arguments!\n");
		exit(WRONG_NO_ARG);
	}

	// process and store parameters
    char BASE_PATH[strlen(argv[1])];
	strcpy(BASE_PATH, argv[1]);

	int dirPathLength = strlen(argv[2]);
	char *DST_DIRPATH = (char *)malloc(sizeof(char) * (dirPathLength + 1));
	if (argv[2][dirPathLength-1] == '/')
	{
		strcpy(DST_DIRPATH, argv[2]);
	}
	else
	{
		sprintf(DST_DIRPATH, "%s%s", argv[2], "/");
	}

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
	changeHostToDnsFormat(hostDnsFormat, (unsigned char *)BASE_PATH);

	FILE *file;
	char *DST_PATH;  // destination path (dir path + file path from sender)
	char *ptr;  // pointer to BASE_HOST sustring
	char encodedData[253];
	char decodedData[253];
	
	while(1)
	{
		int n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, MSG_WAITALL, (struct sockaddr *)&socketAddr, (socklen_t *)&length);

		char client_addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), client_addr_str, INET_ADDRSTRLEN);
		
		// TODO: delete
		printf("---------------------------\nReceived %d bytes from %s\n", n, client_addr_str);
		print_buffer(buffer, n);
		
		char *dnsQuery = (char *)&buffer[sizeof(struct dns_header)];
		ptr = strstr(dnsQuery, (char *)hostDnsFormat);
		int dataLength = 0;
		if (ptr)
		{
			dataLength = (int)(ptr-dnsQuery);
		}
		else {continue;}

		memset(encodedData, 0, 253);
		int start = 1;
		int labelLength = (dataLength >= 63) ? 63 : dataLength;

		while (start < dataLength)
		{
			labelLength = (dataLength >= start + 63) ? 63 : (dataLength - start);

			strncat(encodedData, dnsQuery+start, labelLength);
			// +1, because of number which indicates the length of label
			start += labelLength+1;
		}

		// decode data
		memset(decodedData, 0, 253);
		int lengthOfDecodedData = base32_decode((u_int8_t*)encodedData, (u_int8_t*)decodedData, strlen(encodedData));

		// init packet
		if (strncmp(decodedData, "DST_FILEPATH[", 13) == 0)
		{
			if(DST_PATH) free(DST_PATH);

			char *startPos = strstr(decodedData, "[");
			char *endPos = strstr(decodedData, "]");
			if (startPos && endPos)
			{
				int length = endPos - startPos - 1;

				DST_PATH = malloc(sizeof(char) * (length + dirPathLength));
				sprintf(DST_PATH, "%s", DST_DIRPATH);
				strncat(DST_PATH, startPos+1, length);
			}

			// separate dir path and file name
			int lastPos = 0;
			for (int i = 0; i < strlen(DST_PATH); i++)
			{
				if (DST_PATH[i] == '/')
				{
					lastPos = i;
				}
			}
			char createPath[lastPos+1];
			memset(createPath, 0, lastPos+1);
			strncpy(createPath, DST_PATH, lastPos);

			// create dir
			char command[100];
			sprintf(command, "mkdir -p %s", createPath);
			system(command);
		
			// open file
			file = fopen(DST_PATH, "w");
		}
		// end packet
		else if (strcmp(decodedData, "[END_CONNECTION]") == 0)
		{
			fclose(file);
		}
		// data
		else
		{
			fprintf(file, decodedData);
		}

		memset(dnsQuery, 0, 253);
		memset(buffer, 0, MAX_BUFF_SIZE);

		// TODO: send response
		// if (sendto(sockfd, buffer, response_length, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1)
		// {
		// 	fprintf(stderr, "ERROR: Send response to client!\n");
		// 	exit(1);
		// }
    }

	return RECEIVER_OK;
}