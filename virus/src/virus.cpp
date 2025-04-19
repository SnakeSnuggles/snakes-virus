#include <iostream>
#include <asio.hpp>
#include <windows.h>

using asio::ip::tcp;

enum packet_id {
    GET_DATA = 0,
    POPUP_1 = 1
};

#pragma pack(push, 1)
struct Packet {
    packet_id id;
    union {
        int number;
        double double_number;
        char str[256];
    };

    Packet() : id(GET_DATA) { // Default constructor
        std::memset(str, 0, sizeof(str)); // Ensure str is initialized
    }
};
#pragma pack(pop)
void show_popup(const char* message) {
    MessageBoxA(NULL, message, "Popup", MB_OK | MB_ICONINFORMATION);
}
int main() {
    try {
        asio::io_context io_context;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "1234");

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        while (true) {
            Packet packet;
            std::size_t len = asio::read(socket, asio::buffer(&packet, sizeof(Packet)));

            // Check that we read the full packet
            if (len == sizeof(Packet)) {
                switch (packet.id) {
                    case POPUP_1:
                        show_popup(packet.str);
                        break;
                    default:
                        break;
                }
            } else {
                std::cerr << "Error: Incomplete packet received." << std::endl;
            }
        }

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
