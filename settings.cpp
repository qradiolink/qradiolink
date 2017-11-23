#include "settings.h"

Settings::Settings()
{
    _id = 0;
    _use_mumble = 1;
    _mumble_tcp = 1; // used
    _use_codec2 = 0; // used
    _use_dtmf = 0; // used
    _audio_treshhold = -15; // used
    _voice_activation = 0.5; // used
    _voice_activation_timeout = 50; // used
    _voice_server_port = 64738;
    _local_udp_port = 4938;
    _control_port = 4939;
    _opus_bitrate = 8000;
    _codec2_bitrate =1200;
    _enable_vox = 0; // unused
    _enable_agc = 0; // unused
    _ident_time = 300; // used
    _radio_id = "";
    _callsign = "";
    _voice_server_ip="127.0.0.1";
}
