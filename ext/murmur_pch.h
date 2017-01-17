#ifndef Q_MOC_RUN
#ifndef MUMBLE_MURMUR_MURMUR_PCH_H_
#define MUMBLE_MURMUR_MURMUR_PCH_H_


#define _USE_MATH_DEFINES
#if defined(__INTEL_COMPILER)
#include <mathimf.h>
#endif



#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>
#include <QtSql/QtSql>

#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
/* OpenSSL defines set_key. This breaks our protobuf-generated setters. */
#undef set_key

#ifdef Q_OS_UNIX
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifdef Q_OS_LINUX
#include <linux/types.h> // needed to work around evil magic stuff in capability.h
//#include <sys/capability.h>
#include <sys/prctl.h>
#endif
#include <pwd.h>
#include <grp.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#if !defined (Q_CC_INTEL) && !defined (Q_OS_WIN)
#include <math.h>
#endif

#define STACKVAR(type, varname, count) type varname[count]


//#include <boost/function.hpp>
//#include <boost/bind.hpp>
//#include <boost/shared_ptr.hpp>
//#include <boost/weak_ptr.hpp>


#endif
#endif
