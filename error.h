/**
 * @file error.h
 * @author Patrik Sehnoutek (xsehno01@stud.fit.vutbr.cz)
 * @brief Error definitions.
 * @date 2022-10-15
 */


#ifndef _ERROR_H
#define _ERROR_H


// Error codes
#define RECEIVER_OK         0  // receiver ends without error
#define SENDER_OK           0  // sender ends without error
#define CREATE_SOCK_ERR     1  // create socket error 
#define SOCK_OPT_ERR        2  // set socket options error
#define BIND_SOCK_ERR       3  // bind socket error
#define WRONG_NO_ARG        4  // wrong number of arguments
#define SEND_TO_ERR         5  // send to error
#define RECV_FROM_ERR       6  // receive from error
#define INTERNAL_ERR        99  // internal error, eg. malloc or free error


#endif //_ERROR_H