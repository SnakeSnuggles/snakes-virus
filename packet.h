#pragma once

enum packet_id {
    GET_DATA,
    POPUP,
    OPEN_LINK,
    TTS,
    CURSER_POS,
    RANDOM_MOUSE_MOVE,
    INVERT_MOUSE,
    LOCK_MOUSE,
    FREE_MOUSE,
    REQUEST_VIDEO_FRAME,
    SEND_KEYBOARD,
    ECHO_KEYBOARD,
};

#pragma pack(push, 1)
struct Packet {
    packet_id id;
    union {
        int number;
        bool bo;
        int x, y, size_x, size_y;
        packet_id packetid;
        double double_number;
        char str[256];
    };
    
};
#pragma pack(pop)
