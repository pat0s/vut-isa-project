#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include "dns_sender_events.h"
#include "../error.h"
#include "../dns.h"


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

int main(int argc, char* argv[])
{
	// check and process paramaters
	char *UPSTREAM_DNS_IP;
	char *BASE_PATH;
	char *DST_FILEPATH;
	char SRC_FILEPATH[100] = "stdin";

	// check parameters 
	int errCode = checkParameters(argc, argv, &UPSTREAM_DNS_IP, &BASE_PATH, &DST_FILEPATH, SRC_FILEPATH);
	if (errCode)
	{
		fprintf(stderr, "Wrong number of arguments!\n");
		exit(errCode);
	}

	printf("%s", UPSTREAM_DNS_IP);
	printf("%s", BASE_PATH);
	printf("%s", DST_FILEPATH);
	printf("%s", SRC_FILEPATH);

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

	// TODO: MSG_CONFIRM ?
	errCode = sendto(sockfd, buffer, MAX_BUFF_SIZE, MSG_CONFIRM, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
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
