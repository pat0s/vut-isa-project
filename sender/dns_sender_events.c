/**
 * @file dns_sender_events.c
 * @author Patrik Sehnoutek (xsehno01@stud.fit.vutbr.cz)
 * @brief UDP client for DNS tunneling
 * @date 2022-10-24
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "dns_sender_events.h"
#include "../error.h"
#include "../dns.h"
#include "../base32.h"

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

int getSystemDnsServer(char **ip)
{
	char dnsServer[MAX_ARG_LENGTH+1];
	FILE *file;

	file = popen("cat /etc/resolv.conf | grep nameserver | head -1 | awk -F' ' '{print $2}'" , "r");
	if (file == NULL) return 1;

	fgets(dnsServer, MAX_ARG_LENGTH, file);
    // remove /n from fgets
    dnsServer[strcspn(dnsServer, "\r\n")] = 0;
	pclose(file);

	*ip = (char *)malloc(sizeof(char) * strlen(dnsServer));
	strcpy(*ip, dnsServer);

	return 0;
}

int checkParameters(int argc, char* argv[], char** dnsIP, char** baseHost, char* dst, char* srcPath)
{
	// check number of paramters
	if (argc < 3 || argc > 6) return WRONG_NO_ARG;

	if (strcmp(argv[1], "-u") == 0)
	{
		if (argc < 5) return WRONG_NO_ARG;

		*dnsIP = (char*)malloc(sizeof(char) * strlen(argv[2]));
		strcpy(*dnsIP, argv[2]);
		*baseHost = argv[3];
		strcpy(dst, argv[4]);

		if (argc == 6)
		{
			strcpy(srcPath, argv[5]);
		}
	}
	else
	{
		if (argc > 4) return WRONG_NO_ARG;

		*baseHost = argv[1];
		strcpy(dst, argv[2]);

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
	char *UPSTREAM_DNS_IP = NULL;
	char *BASE_HOST = NULL;
	char DST_FILEPATH[MAX_ARG_LENGTH] = "\0";
	char SRC_FILEPATH[MAX_ARG_LENGTH] = "\0";

	// check parameters 
	int errCode = checkParameters(argc, argv, &UPSTREAM_DNS_IP, &BASE_HOST, DST_FILEPATH, SRC_FILEPATH);
	if (errCode)
	{
		fprintf(stderr, "Wrong number of arguments!\n");
		exit(errCode);
	}

	// load default dns from system
	if (!UPSTREAM_DNS_IP)
	{
		if (getSystemDnsServer(&UPSTREAM_DNS_IP) == 1)
		{
			fprintf(stderr, "ERORR: Cannot load dns nameserver from system!\n");
			exit(INTERNAL_ERR);
		}
	}

	fprintf(stderr, "%s\n", UPSTREAM_DNS_IP);

	// create socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
	{
		fprintf(stderr, "ERROR: Cannot create socket!\n");
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

	struct timeval timeval;
	timeval.tv_sec = 3;
	timeval.tv_usec = 0;
	err = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeval, sizeof timeval);
	if (err < 0)
	{
		fprintf(stderr, "ERROR: Cannot set timeoptions on socket\n");
		exit(SOCK_OPT_ERR);
	}

	struct sockaddr_in socketAddr, serverAddr;
	socketAddr.sin_addr.s_addr = inet_addr(UPSTREAM_DNS_IP);
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(DNS_PORT);

	unsigned char buffer[MAX_BUFF_SIZE];
  	memset(buffer, 0, MAX_BUFF_SIZE);

	struct dns_header *dnsHeader;
	dnsHeader = (struct dns_header*)&buffer;
	int packetId = getpid();

	dnsHeader->id = (unsigned short) htons(packetId++);
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

	// Load data from file or stdin
	FILE *file = strlen(SRC_FILEPATH) ? fopen(SRC_FILEPATH, "r") : stdin;
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s for reading.\n", SRC_FILEPATH);
		exit(INTERNAL_ERR);
	} 

	// struct for input data
	struct rawInput inputData;
	inputData.data = (char *)malloc(2*sizeof(char));
	inputData.length = 2;
	inputData.currentPos = 0;
	
	// load data
	char value;
	while( (value = fgetc(file)) != EOF )
    {
		inputData.data[inputData.currentPos++] = value;

		if (inputData.length == inputData.currentPos)
		{
			inputData.length *= 2;
			inputData.data = (char *)realloc(inputData.data, inputData.length);
		}
    }

	// close file and add \0 to the end of array
	fclose(file);
	inputData.data[inputData.currentPos] = '\0';

	// change BASE_HOST to dns format
	unsigned char hostDnsFormat[253] = {'\0'}; 
	changeHostToDnsFormat(hostDnsFormat, (unsigned char *)BASE_HOST);

	// 253 free space - length of converted BASE_HOST + 1 (\0)
	int freeSpace = 253 - (strlen((const char*)qname) + 1);
	// -4, 4-krat sa tam zmesti cislo indikujuce pocet znakov za nim
	int freeSpaceDecoded = BASE32_LENGTH_DECODE(freeSpace-4);

	int start = 0;
	int end, noChars, lengthOfEncodedData;
	u_int8_t decodedData[253];
	u_int8_t base32Data[253];

	// init packet
	memset(decodedData, 0, 253);
	memset(base32Data, 0, 253);

	int initPacket = 1;
	int endPacket = 0;
	int fileSize = 0;
	const char initPacketMsg[17] = "DST_FILEPATH[%s]";
	const char endPacketMsg[17] = "[END_CONNECTION]";

	unsigned char responseBuff[MAX_BUFF_SIZE] = {'\0'};
	unsigned short responded = 0;
	socklen_t length = sizeof(serverAddr);

	while (!endPacket)
	{
		if (start >= inputData.currentPos)
		{
			endPacket = 1;
			initPacket = 0;
		}

		if (initPacket)
		{
			sprintf((char *)decodedData, initPacketMsg, DST_FILEPATH);
			lengthOfEncodedData = base32_encode((u_int8_t*)decodedData, strlen((const char *)decodedData), (u_int8_t*)base32Data, BASE32_LENGTH_ENCODE(strlen((const char *)decodedData)));
			
			// follow to DNS_QNAME format
			*(qname) = (unsigned char)lengthOfEncodedData;
			strcat((char *)qname, (const char*)base32Data);
		}
		else if (endPacket)
		{
			strcat((char *)decodedData, endPacketMsg);
			lengthOfEncodedData = base32_encode((u_int8_t*)decodedData, strlen((const char *)decodedData), (u_int8_t*)base32Data, BASE32_LENGTH_ENCODE(strlen((const char *)decodedData)));
			
			// follow to DNS_QNAME format
			*(qname) = (unsigned char)lengthOfEncodedData;
			strcat((char *)qname, (const char*)base32Data);
		}
		else
		{
			end = (start+freeSpaceDecoded <= inputData.currentPos)? start+freeSpaceDecoded : inputData.currentPos;
			noChars = end-start;

			memcpy(decodedData, inputData.data+start, noChars);
			start += noChars;

			// count input file size in B
			fileSize += strlen((const char *)decodedData);

			// encode input data
			lengthOfEncodedData = base32_encode((u_int8_t*)decodedData, noChars, (u_int8_t*)base32Data, BASE32_LENGTH_ENCODE(noChars));

			// divide to labels (max length of label is 63)
			int startLabel = 0;
			int endLabel, noLabelChars;
			int i = 0;

			freeSpace = (freeSpace <= lengthOfEncodedData) ? freeSpace : lengthOfEncodedData;

			while (startLabel < freeSpace)
			{
				endLabel = (startLabel + 63 <= freeSpace) ? (startLabel+63) : freeSpace;
				noLabelChars = endLabel - startLabel;
				
				*(qname+startLabel+i) = (unsigned char)noLabelChars;
				strncat((char *)qname, (const char*)base32Data+startLabel, noLabelChars);
				
				startLabel += noLabelChars;
				i++;
			}
		}

		// memset to 0
		memset(decodedData, 0, noChars);
		memset(base32Data, 0, noChars);

		strcat((char *)qname, (const char*)hostDnsFormat);

		uint16_t *qinfo = (uint16_t *)&buffer[sizeof(struct dns_header) + strlen((const char*)qname)+1];
		*qinfo = htons(1);  // qtype
		*(qinfo + 2) = htons(1);  // qclass

		size_t bufferSize = sizeof(struct dns_header) + (strlen((const char*)qname)+1) + sizeof(uint16_t)*2;

		responded = 0;
		while (!responded)
		{
			errCode = sendto(sockfd, buffer, bufferSize, MSG_CONFIRM, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
			if (errCode < 0)
			{
				fprintf(stderr, "Error: sendto\n");
				exit(SEND_TO_ERR);
			}

			// call helper functions for testing
			if (initPacket) 
			{
				dns_sender__on_transfer_init(&(socketAddr.sin_addr));
			}
			else if (endPacket) 
			{ 
				dns_sender__on_transfer_completed(DST_FILEPATH, fileSize); 
			}
			else
			{
				dns_sender__on_chunk_encoded(DST_FILEPATH, dnsHeader->id, (char *)qname);
				dns_sender__on_chunk_sent(&(socketAddr.sin_addr), DST_FILEPATH, dnsHeader->id, strlen((const char*)decodedData));
			}

			short receivedBytes = recvfrom(sockfd, responseBuff, MAX_BUFF_SIZE, MSG_WAITALL, (struct sockaddr *)&serverAddr, &length);
			if (receivedBytes <= 0)
			{
				fprintf(stderr, "Resending DNS packet\n");
				continue;
			}

			// compare IDs of sent and received DNS header
			struct dns_header *responseDnsHeader = (struct dns_header*)&responseBuff;
			if (dnsHeader->id == responseDnsHeader->id) responded = 1;
		}

		if (initPacket) initPacket = 0;
		if (endPacket) fileSize = 0;

		// memset qname
		memset(qname, 0, strlen((const char *)qname)+2);
		// set different id to dns header
		dnsHeader->id = (unsigned short) htons(packetId++);
	}

	return SENDER_OK;
}