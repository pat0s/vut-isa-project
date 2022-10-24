# ISA project 2022/2023
## Tunneling of data transmissions via DNS queries

**Author:** Patrik Sehnoutek\
**Login:** xsehno01\
**Date:** 24.10.2022

## Description
This project focuses on the implementation of an application for tunneling data through DNS queries. It implements client and server application.

## Usage
- run everything in the root folder of the project
```bash
$ make

$ sudo ./dns_receiver {BASE_HOST} {DST_DIRPATH}
     
       {BASE_HOST}          is used to set the base domain to receive data
       {DST_DIRPATH}        path under which all incoming data/files will be saved (the path specified by the client will be created under this directory)

$ sudo ./dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]

       -u UPSTREAM_DNS_IP   is used to force a remote DNS server, default value: DNS server set in the system
       {BASE_HOST}          is used to set the base domain of all transfers, i.e. queries will be sent to addresses *.{BASE_HOST}
       {DST_FILEPATH}       path under which the data will be saved on server
       [SRC_FILEPATH]       path to the file that will be sent, if not specified applicaton reads data from STDIN
```

## Usage examples
Open two terminals. You can run client application as many times as you want.

**1. run server application**
```bash
$ sudo ./dns_receiver example.com test-directory/
```
**2. run client application**
```bash
$ sudo -u 127.0.0.1 ./dns_sender example.com output-data.txt input-data.txt
```
```bash
$ sudo ./dns_sender example.com output-data.txt
```

## Restrictions
* application doesn't support IPv6 protocol

## List of uploaded files
* **receiver/dns_receiver_events.c**
* **receiver/dns_receiver_events.h**
* **sender/dns_sender_events.c**
* **sender/dns_sender_events.h**
* **base32.c**
* **base32.h**
* **dns.h**
* **error.h**
* **Makefile**
* **README.md**
* **manual.pdf**
