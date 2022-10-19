#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include "dns_sender_events.h"
#include "../error.h"
#include "../dns.h"
#include "../base32.h"


#define MAX_DNS_SERVER_IP 100

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

int checkParameters(int argc, char* argv[], char** dnsIP, char** basePath, char** dst, char* srcPath)
{
	// check number of paramters
	if (argc < 3 || argc > 6) return WRONG_NO_ARG;

	if (strcmp(argv[1], "-u") == 0)
	{
		// TODO: error message
		if (argc < 5) return WRONG_NO_ARG;

		*dnsIP = argv[2];
		*basePath = argv[3];
		*dst = argv[4];

		if (argc == 6)
		{
			strcpy(srcPath, argv[5]);
		}
	}
	else
	{
		if (argc > 4) return WRONG_NO_ARG;

		*basePath = argv[1];
		*dst = argv[2];

		if (argc == 4)
		{
			strcpy(srcPath, argv[3]);
		}
	}

	return 0;
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
	// check and process paramaters
	char *UPSTREAM_DNS_IP;
	char *BASE_PATH;
	char *DST_FILEPATH;
	char SRC_FILEPATH[100] = "\0";

	// check parameters 
	int errCode = checkParameters(argc, argv, &UPSTREAM_DNS_IP, &BASE_PATH, &DST_FILEPATH, SRC_FILEPATH);
	if (errCode)
	{
		fprintf(stderr, "Wrong number of arguments!\n");
		exit(errCode);
	}

	printf("%s\n", UPSTREAM_DNS_IP);
	printf("%s\n", BASE_PATH);
	printf("%s\n", DST_FILEPATH);
	printf("%s\n", SRC_FILEPATH);

	// load default dns from system
	if (!UPSTREAM_DNS_IP)
	{
		char dnsServer[MAX_DNS_SERVER_IP+1];

		if (getSystemDnsServer(dnsServer) == 1)
		{
			fprintf(stderr, "Cannot load dns nameserver from system!\n");
			exit(1);
		}

		UPSTREAM_DNS_IP = dnsServer;
	}


	// create socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
	{
		// TODO: error message
		exit(CREATE_SOCK_ERR);
	}

	// set socket option
	const int enabled = 1;
	int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
	if (err < 0)
	{
		fprintf(stderr, "ERROR: Cannot set options on socket\n");
		exit(SOCK_OPT_ERR);
	}

	struct sockaddr_in socketAddr;
	socketAddr.sin_addr.s_addr = inet_addr(UPSTREAM_DNS_IP);
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(DNS_PORT);

	unsigned char buffer[MAX_BUFF_SIZE];
  	memset(buffer, 0, MAX_BUFF_SIZE);

	struct dns_header *dnsHeader;
	dnsHeader = (struct dns_header*)&buffer;

	dnsHeader->id = (unsigned short) htons(getpid());
	dnsHeader->qr = 0; 
	dnsHeader->opcode = 0;
	dnsHeader->aa = 0;
	dnsHeader->tc = 0;
	dnsHeader->rd = 1;
	dnsHeader->ra = 0; 
	dnsHeader->z = 0;
	dnsHeader->ad = 0;
	dnsHeader->cd = 0;
	dnsHeader->rcode = 0;
	dnsHeader->q_count = htons(1);
	dnsHeader->ans_count = 0;
	dnsHeader->auth_count = 0;
	dnsHeader->add_count = 0;

	unsigned char *qname = (unsigned char*)&buffer[sizeof(struct dns_header)];

	FILE *file = strlen(SRC_FILEPATH) ? fopen(SRC_FILEPATH, "r") : stdin;
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s for reading.\n", SRC_FILEPATH);
		exit(INTERNAL_ERR);
	} 

	char buff[201];
	fgets(buff, 200, file);
	printf("%s", buff);

	fclose(file);

	//int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize) 

	// change BASE_HOST to dns format
	changeHostToDnsFormat(qname, BASE_PATH);

	// 253 free space - length of converted BASE_HOST + 1 (\0)
	int freeSpace = 253 - (strlen((const char*)qname) + 1);
	printf("%d", freeSpace);

	uint16_t *position = (uint16_t *)&buffer[sizeof(struct dns_header) + strlen((const char*)qname)+1];
	*position = htons(1);  // qtype
	*(position + 2) = htons(1);  // qclass

	size_t bufferSize = sizeof(struct dns_header) + (strlen((const char*)qname)+1) + sizeof(uint16_t)*2;

	errCode = sendto(sockfd, buffer, bufferSize, MSG_CONFIRM, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
	if (errCode < 0)
	{
		fprintf(stderr, "Error: sendto\n");
		exit(1);
	}

  	// unsigned char responseBuff[MAX_BUFF_SIZE];
  	// int num_received;
  	// socklen_t socklen = sizeof(struct sockaddr_in);
  	// if ((num_received = recvfrom(sockfd, response, sizeof(response), MSG_WAITALL, (struct sockaddr *)&sockaddr, &socklen)) == -1) {

	return 0;
}