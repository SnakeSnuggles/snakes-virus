#include <random>
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <sapi.h>
#include <winnt.h>

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
