/**
 * @brief Error definitions.
 *
 * @author Patrik Sehnoutek <xsehno01@stud.fit.vutbr.cz>
 */


#ifndef _ERROR_H
#define _ERROR_H


// Error codes
#define RECEIVER_OK         0  // receiver ends without error
#define SENDER_OK           0  // sender ends without error
#define CREATE_SOCK_ERR     1  // create socket error 
#define SOCK_OPT_ERR        2  // set socket options error
#define BIND_SOCK_ERR       3  // bind socket error
#define INTERNAL_ERR        99  // internal error, eg. malloc or free error


#endif //_ERROR_H