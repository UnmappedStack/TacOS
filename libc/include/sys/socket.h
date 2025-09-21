#pragma once

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define SOCK_STREAM 0

int socket(int domain, int type, int protocol);
