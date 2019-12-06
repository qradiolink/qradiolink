#ifndef CONFIG_DEFINES_H
#define CONFIG_DEFINES_H

#define LOCAL // for testing purposes
#define NO_CRYPT 1 // send UDP voice unencrypted

#define MUMBLE_PORT 64738
#define MUMBLE_TCP_AUDIO 1 // send voice via TCP SSL?

// Old one was 1.2.8
#define PROTVER_MAJOR 1
#define PROTVER_MINOR 2
#define PROTVER_PATCH 8
#define PROTOCOL_VERSION ((PROTVER_MAJOR << 16) | (PROTVER_MINOR << 8) | (PROTVER_PATCH))

#define CONTROL_PORT 4939
#ifdef LOCAL
#define UDP_PORT 0 // multiple clients on local station can't bind to the same port
#else
#define UDP_PORT 4938
#endif

#define MAX_NUM_USERS 4

#endif // CONFIG_DEFINES_H
