#ifndef DEC_H
#define DEC_H

typedef enum {
	Version,
	UDPTunnel,
	Authenticate,
	Ping,
	Reject,
	ServerSync,
	ChannelRemove,
	ChannelState,
	UserRemove,
	UserState,
	BanList, /* 10 */
	TextMessage,
	PermissionDenied,
	ACL,
	QueryUsers,
	CryptSetup,
	ContextActionAdd,
	ContextAction,
	UserList,
	VoiceTarget,
	PermissionQuery, /* 20 */
	CodecVersion,
	UserStats,
	RequestBlob,
	ServerConfig
} messageType_t; 

typedef enum {
	UDPVoiceCELTAlpha,
	UDPPing,
	UDPVoiceSpeex,
	UDPVoiceCELTBeta,
	UDPVoiceOpus,
} UDPMessageType_t;


typedef enum {
	Parameters,
	JoinConference,
	LeaveConference
} QRadioLinkType_t;
	

#endif
