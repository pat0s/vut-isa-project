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

/**
 * @brief Check and store parameters.
 * 
 * @param argc 
 * @param argv 
 * @param baseHost 
 * @param dstDirPath 
 * @return int 
 */
int checkParameters(int argc, char* argv[], char *baseHost, char **dstDirPath)
{
	// 2 mandatory parameters - BASE_PATH, DST_DIRPATH
	if (argc != 3) return WRONG_NO_ARG;

	strcpy(baseHost, argv[1]);

	int dirPathLength = strlen(argv[2]);
	*dstDirPath = (char *)malloc(sizeof(char) * (dirPathLength + 1));
	if (argv[2][dirPathLength-1] == '/')
	{
		strcpy(*dstDirPath, argv[2]);
	}
	else
	{
		sprintf(*dstDirPath, "%s%s", argv[2], "/");
	}

	return RECEIVER_OK;
}

/**
 * @brief Convert domain name (BASE_HOST) to DNS acceptable format.
 * 
 * @param dnsBuffer 
 * @param host
 * 
 * https://www.binarytides.com/dns-query-code-in-c-with-linux-sockets/
 */
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

/**
 * @brief Seperate directory path and file name. Create a directory.
 * 
 * @param DST_PATH 
 */
void createDirectory(char *DST_PATH)
{
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
}

/**
 * @brief Concatenate directory path with file path from client.
 * 
 * @param DST_PATH 
 * @param DST_DIRPATH 
 * @param decodedData 
 */
void getFullPath(char **DST_PATH, char *DST_DIRPATH, char *decodedData)
{
	char *startPos = strstr(decodedData, "[");
	char *endPos = strstr(decodedData, "]");
	if (startPos && endPos)
	{
		int length = endPos - startPos - 1;

		*DST_PATH = malloc(sizeof(char) * (length + strlen(DST_DIRPATH)));
		sprintf(*DST_PATH, "%s", DST_DIRPATH);
		strncat(*DST_PATH, startPos+1, length);
	}
}

/**
 * @brief Send reponse to client.
 * 
 * @param sockfd 
 * @param clientAddr 
 * @param dnsQuery 
 * @param headerId 
 * @return int 
 */
int sendResponse(int sockfd, struct sockaddr_in clientAddr, char *dnsQuery, int headerId, char * ipAdress)
{
	clientAddr.sin_addr.s_addr = inet_addr(ipAdress);

	char bufferResponse[MAX_BUFF_SIZE] = {'\0'};
	struct dns_header *dnsHeader = (struct dns_header*)&bufferResponse;

	dnsHeader->id = headerId;
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
	dnsHeader->ans_count = htons(1);
	dnsHeader->auth_count = 0;
	dnsHeader->add_count = 0;

	unsigned char* qname = (unsigned char*)&bufferResponse[sizeof(struct dns_header)];
	strcpy((char *)qname, dnsQuery);
	uint16_t *qinfo = (uint16_t *)&bufferResponse[sizeof(struct dns_header) + strlen((const char*)qname)+1];
	*qinfo = htons(1);  // qtype
	*(qinfo + 2) = htons(1);  // qclass

	struct dns_answer *dnsAnswer = (struct dns_answer *)&bufferResponse[sizeof(struct dns_header) + (strlen((const char*)qname)+1) + sizeof(uint16_t)*2];

	dnsAnswer->ans_type = htons(1);
	dnsAnswer->name_offset = 0;
	dnsAnswer->type = htons(1);
	dnsAnswer->qclass = htons(1);
	dnsAnswer->ttl = 0;
	dnsAnswer->rdlength = 0;
	dnsAnswer->rdata = 0;

	size_t bufferSize = sizeof(struct dns_header) + (strlen((const char*)qname)+1) + sizeof(uint16_t)*2 + sizeof(struct dns_answer);

	if (sendto(sockfd, bufferResponse, bufferSize, MSG_CONFIRM, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0)
	{
		fprintf(stderr, "Error: sendto");
		return SEND_TO_ERR;
	}

	return RECEIVER_OK;
}

int main(int argc, char* argv[])
{
	// process and store parameters
	char BASE_PATH[strlen(argv[1])];
	char *DST_DIRPATH = NULL;
	
	// check and process parameters
	int err = checkParameters(argc, argv, BASE_PATH, &DST_DIRPATH);
	if (err != RECEIVER_OK)
	{
		fprintf(stderr, "Wrong number of arguments!\n");
		exit(err);
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
	err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
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
	socklen_t length = sizeof(clientAddr);

	unsigned char hostDnsFormat[253] = {'\0'}; 
	changeHostToDnsFormat(hostDnsFormat, (unsigned char *)BASE_PATH);

	FILE *file;
	char *DST_PATH;  // destination path (dir path + file path from sender)
	char *ptr;  // pointer to BASE_HOST sustring
	
	struct dns_header *dnsHeader;
	char *dnsQuery;
	char encodedData[253];
	char decodedData[253];
	int fileSize = 0;
	
	while(1)
	{
		int n = recvfrom(sockfd, buffer, MAX_BUFF_SIZE, MSG_WAITALL, (struct sockaddr *)&clientAddr, &length);

		char clientAddrStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientAddrStr, INET_ADDRSTRLEN);
		
		dnsHeader = (struct dns_header*)&buffer;
		dnsQuery = (char *)&buffer[sizeof(struct dns_header)];

		// check if BASE_HOST form client and server match
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
			// set default values
			fileSize = 0;
			if(DST_PATH) free(DST_PATH);

			// concatenate dir path and file path from client
			getFullPath(&DST_PATH, DST_DIRPATH, decodedData);

			// create dir
			createDirectory(DST_PATH);
		
			// open file
			file = fopen(DST_PATH, "w");

			// init connection with client
			dns_receiver__on_transfer_init(&(clientAddr.sin_addr));
		}
		// end packet
		else if (strncmp(decodedData, "[END_CONNECTION]", 16) == 0)
		{
			fclose(file);

			// end transfer of one file
			dns_receiver__on_transfer_completed(DST_PATH, fileSize);
		}
		// data packets
		else
		{
			// calculate length of received data
			fileSize += lengthOfDecodedData;
			// print data to file
			fprintf(file, "%s", decodedData);

			dns_receiver__on_query_parsed(DST_PATH, dnsQuery);
			dns_receiver__on_chunk_received(&(clientAddr.sin_addr), DST_PATH, dnsHeader->id, strlen(decodedData));
		}

		// send answer to client
		err = sendResponse(sockfd, clientAddr, dnsQuery, dnsHeader->id, clientAddrStr);
		if (err != RECEIVER_OK) exit(err);

		memset(dnsQuery, 0, 253);
		memset(buffer, 0, MAX_BUFF_SIZE);
    }

	return RECEIVER_OK;
}