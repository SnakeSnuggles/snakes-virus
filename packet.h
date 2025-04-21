#pragma once

enum packet_id {
    GET_DATA = 0,
    POPUP = 1,
    OPEN_LINK = 2,
    TTS = 3,
    CURSER_POS = 4,
    COUNT,
};

#pragma pack(push, 1)
struct Packet {
    packet_id id;
    union {
        int number;
        int x, y;
        packet_id packetid;
        double double_number;
        char str[256];
    };
    
};
#pragma pack(pop)
