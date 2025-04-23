#pragma once
#include <windows.h>
#include <string>
#include <iostream>

// ──────────────────────────────────────────────────────────────────────────────
//  tiny helper – press / release one UTF‑16 code unit via SendInput
// ──────────────────────────────────────────────────────────────────────────────
static void send_utf16_cu(char16_t cu, bool key_up = false)
{
    INPUT in{};
    in.type            = INPUT_KEYBOARD;
    in.ki.dwFlags      = KEYEVENTF_UNICODE | (key_up ? KEYEVENTF_KEYUP : 0);
    in.ki.wScan        = static_cast<WORD>(cu);   // UTF‑16 code unit
    in.ki.wVk          = 0;                       // must be 0 with KEYEVENTF_UNICODE
    SendInput(1, &in, sizeof(INPUT));
}

// ──────────────────────────────────────────────────────────────────────────────
//  kick a whole UTF‑16 string out to the active window
// ──────────────────────────────────────────────────────────────────────────────
void type_string(const std::u16string& text, unsigned perKeyDelayMs = 10)
{
    for (char16_t cu : text)
    {
        send_utf16_cu(cu);              // key‑down
        send_utf16_cu(cu, true);        // key‑up
        if (perKeyDelayMs) Sleep(perKeyDelayMs);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  convenience wrapper: UTF‑8 → UTF‑16 and call the version above
// ──────────────────────────────────────────────────────────────────────────────
void type_string(const std::string& utf8, unsigned perKeyDelayMs = 10)
{
    if (utf8.empty()) return;

    int n = MultiByteToWideChar(CP_UTF8, 0,
                                utf8.data(), static_cast<int>(utf8.size()),
                                nullptr, 0);
    if (n <= 0) return;

    std::u16string u16(static_cast<size_t>(n), u'\0');
    MultiByteToWideChar(CP_UTF8, 0,
                        utf8.data(), static_cast<int>(utf8.size()),
                        reinterpret_cast<LPWSTR>(u16.data()), n);

    type_string(u16, perKeyDelayMs);
}

// ──────────────────────────────────────────────────────────────────────────────
//  packet handler: just forwards packet.str to our type_string()
// ──────────────────────────────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────────────
//  low‑level keyboard hook (diagnostic only)
// ──────────────────────────────────────────────────────────────────────────────
HHOOK g_hook = nullptr;

LRESULT CALLBACK keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        const auto* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            std::cout << "↓  " << p->vkCode << '\n';
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            std::cout << "↑  " << p->vkCode << '\n';
            break;
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

// ──────────────────────────────────────────────────────────────────────────────
//  translate a WM_KEYDOWN message to its printable UTF‑16 sequence (0–4 bytes)
// ──────────────────────────────────────────────────────────────────────────────
bool msgToUtf16(const MSG& m, std::u16string& out)
{
    if (m.message != WM_KEYDOWN && m.message != WM_SYSKEYDOWN)
        return false;

    const auto* info = reinterpret_cast<KBDLLHOOKSTRUCT*>(m.lParam);
    BYTE  kbState[256]{};
    if (!GetKeyboardState(kbState)) return false;

    WCHAR buf[4]{};                 // room for surrogate pair + NUL
    int n = ToUnicodeEx(static_cast<UINT>(info->vkCode),
                        static_cast<UINT>(info->scanCode),
                        kbState,
                        buf, 4, 0,
                        GetKeyboardLayout(0));

    if (n <= 0) return false;       // dead key or non‑printing

    out.assign(reinterpret_cast<char16_t*>(buf),
               reinterpret_cast<char16_t*>(buf + n));
    return true;
}
