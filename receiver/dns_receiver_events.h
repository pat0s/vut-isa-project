#ifndef ISA22_DNS_RECEIVER_EVENTS_H
#define ISA22_DNS_RECEIVER_EVENTS_H

#include <netinet/in.h>

/**
 * Tato metoda je volána serverem (příjemcem) při přijetí zakódovaných dat od klienta (odesílatele).
 * V případě použití více doménových jmen pro zakódování dat, volejte funkci pro každé z nich.
 *
 * @param filePath Cesta k cílovému souboru
 * @param encodedData Zakódovaná data do doménového jména (např.: "acfe2a42b.example.com")
 */
void dns_receiver__on_query_parsed(char *filePath, char *encodedData);

/**
 * Tato metoda je volána serverem (příjemcem) při příjmu části dat od klienta (odesílatele).
 *
 * @param source IPv4 adresa odesílatele
 * @param filePath Cesta k cílovému souboru
 * @param chunkId Identifikátor části dat
 * @param chunkSize Velikost části dat v bytech
 */
void dns_receiver__on_chunk_received(struct in_addr *source, char *filePath, int chunkId, int chunkSize);

/**
 * Tato metoda je volána serverem (příjemcem) při příjmu části dat od klienta (odesílatele).
 *
 * @param source IPv6 adresa odesílatele
 * @param filePath Cesta k cílovému souboru
 * @param chunkId Identifikátor části dat
 * @param chunkSize Velikost části dat v bytech
 */
void dns_receiver__on_chunk_received6(struct in6_addr *source, char *filePath, int chunkId, int chunkSize);

/**
 * Tato metoda je volána serverem (příjemcem) při zahájení přenosu od klienta (odesílatele).
 *
 * @param source IPv4 adresa odesílatele
 */
void dns_receiver__on_transfer_init(struct in_addr *source);

/**
 * Tato metoda je volána serverem (příjemcem) při zahájení přenosu od klienta (odesílatele).
 *
 * @param source IPv6 adresa odesílatele
 */
void dns_receiver__on_transfer_init6(struct in6_addr *source);

/**
 * Tato metoda je volána serverem (příjemcem) při dokončení přenosu jednoho souboru od klienta (odesílatele).
 *
 * @param filePath Cesta k cílovému souboru
 * @param fileSize Celková velikost přijatého souboru v bytech
 */
void dns_receiver__on_transfer_completed(char *filePath, int fileSize);

/**
 * @brief Check and store parameters.
 * 
 * @param argc 
 * @param argv 
 * @param baseHost 
 * @param dstDirPath 
 * @return int 
 */
int checkParameters(int argc, char* argv[], char *baseHost, char **dstDirPath);

/**
 * @brief Convert domain name (BASE_HOST) to DNS acceptable format.
 * 
 * @param dnsBuffer 
 * @param host
 * 
 * https://www.binarytides.com/dns-query-code-in-c-with-linux-sockets/
 */
void changeHostToDnsFormat(unsigned char* dnsBuffer, unsigned char* host) ;

/**
 * @brief Seperate directory path and file name. Create a directory.
 * 
 * @param DST_PATH 
 */
void createDirectory(char *DST_PATH);

/**
 * @brief Concatenate directory path with file path from client.
 * 
 * @param DST_PATH 
 * @param DST_DIRPATH 
 * @param decodedData 
 */
void getFullPath(char **DST_PATH, char *DST_DIRPATH, char *decodedData);

/**
 * @brief Send reponse to client.
 * 
 * @param sockfd 
 * @param clientAddr 
 * @param dnsQuery 
 * @param headerId 
 * @return int 
 */
int sendResponse(int sockfd, struct sockaddr_in clientAddr, char *dnsQuery, int headerId, char * ipAdress);

#endif //ISA22_DNS_RECEIVER_EVENTS_H
