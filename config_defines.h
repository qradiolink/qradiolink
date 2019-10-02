#ifndef CONFIG_DEFINES_H
#define CONFIG_DEFINES_H

#define LOCAL // for testing purposes
#define NO_CRYPT 1 // send UDP voice unencrypted
//#define USE_CODEC2 1 // use Codec2 instead of Opus

#define MUMBLE 1 // use Mumble protocol
#define MUMBLE_PORT 64738
#define MUMBLE_TCP_AUDIO 0 // send voice via TCP SSL?

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

#define NUM_CALLS 4


#ifndef PI
    #define PI 3.14159265358979323844
#endif
#endif // CONFIG_DEFINES_H
