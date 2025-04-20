#include <iostream>
#include <asio.hpp>
#include <thread>
#include <windows.h>

using asio::ip::tcp;

enum packet_id {
    GET_DATA = 0,
    POPUP = 1
};

#pragma pack(push, 1)
struct Packet {
    packet_id id;
    union {
        int number;
        packet_id packetid;
        double double_number;
        char str[256];
    };
    
};
#pragma pack(pop)

void show_popup(const char* message) {
    MessageBoxA(NULL, message, "Popup", MB_OK | MB_ICONINFORMATION);
}
int main() {
    while(true) {
        std::cout << "trying to connect to home\n";
        try {
            asio::io_context io_context;

            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve("70.77.121.59", "1234");

            tcp::socket socket(io_context);
            asio::connect(socket, endpoints);
            std::cout << "connect to home yayayayayayayayyayay\n";

            while (true) {
                Packet packet;
                std::size_t len = asio::read(socket, asio::buffer(&packet, sizeof(Packet)));

                // Check that we read the full packet
                if (len == sizeof(Packet)) {
                    switch (packet.id) {
                        case GET_DATA:
                            // Offload to a separate thread
                            std::thread([&socket, packet]() {
                                if (packet.id == packet_id::GET_DATA) {
                                    Packet alive_packet{};
                                    alive_packet.id = packet_id::GET_DATA;
                                    alive_packet.number = 1;
                                    asio::write(socket, asio::buffer(&alive_packet, sizeof(alive_packet)));
                                }
                            }).detach();                       
                            break;
                        case POPUP:
                            show_popup(packet.str);
                            std::cout << "recived text: " << packet.str << "\n";
                            break;
                        default:
                            break;
                    }
                } else {
                    std::cerr << "Error: Incomplete packet received." << std::endl;
                }
            }

        } catch (std::exception& e) {
            std::cout << "failed to connect to home \n";
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
    return 0;
}
