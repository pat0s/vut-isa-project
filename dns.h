/**
 * @file dns.h
 * @author Patrik Sehnoutek (xsehno01@stud.fit.vutbr.cz)
 * @brief DNS structures and limitation definitions.
 * @date 2022-10-16
 */


#ifndef _DNS_H
#define _DNS_H


#define MAX_ARG_LENGTH 100
#define DNS_PORT 53
#define MAX_BUFF_SIZE 512

#define BASE32_LENGTH_ENCODE(src_size) (((src_size)*8 + 4) / 5)
#define BASE32_LENGTH_DECODE(src_size) (ceil((src_size)) / 1.6)

// DNS header structure
// https://gist.github.com/fffaraz/9d9170b57791c28ccda9255b48315168
struct dns_header
{
	unsigned short id; // identification number

	unsigned char rd :1; // recursion desired
	unsigned char tc :1; // truncated message
	unsigned char aa :1; // authoritive answer
	unsigned char opcode :4; // purpose of message
	unsigned char qr :1; // query/response flag

	unsigned char rcode :4; // response code
	unsigned char cd :1; // checking disabled
	unsigned char ad :1; // authenticated data
	unsigned char z :1; // its z! reserved
	unsigned char ra :1; // recursion available

	unsigned short q_count; // number of question entries
	unsigned short ans_count; // number of answer entries
	unsigned short auth_count; // number of authority entries
	unsigned short add_count; // number of resource entries
};

// DNS answer structure
// https://github.com/tbenbrahim/dns-tunneling-poc/blob/main/src/dns.h
// license: MIT License
struct __attribute__((__packed__)) dns_answer
{
	uint8_t ans_type;
	uint8_t name_offset;
	uint16_t type;
	uint16_t qclass;
	uint32_t ttl;
	uint16_t rdlength;
	uint32_t rdata;
};

struct rawInput
{
	char *data;
	unsigned int length;
	unsigned int currentPos;
};

#endif //_DNS_H