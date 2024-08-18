#include "LuminSOCD.hpp"
#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <optional>
#include <iomanip>
#include <shlwapi.h>

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")


void LuminSOCD::initializeWindow(const HWND handle) {
    main_window = handle;

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(handle, GWLP_HINSTANCE);

    ESC_PRESSED = IS_UP;
    disableKeyPressed = IS_UP;
    std::fill(real.begin(), real.end(), IS_UP);
    std::fill(virtual_keys.begin(), virtual_keys.end(), IS_UP);

    loadConfiguration();

    if (programs_whitelist[0].empty()) {
        installKeyboardHook(hInstance);
    }
    else {
        SetWinEventHook(
            EVENT_OBJECT_FOCUS,
            EVENT_OBJECT_FOCUS,
            hInstance,
            handleFocusChange,
            0,
            0,
            WINEVENT_OUTOFCONTEXT
        );
    }
}

void LuminSOCD::configureKeyBindings(const std::array<Key, 4>& bindings, const Key disableBind, const Key escBind) {
    CUSTOM_BINDS = bindings;
    DISABLE_BIND = disableBind;
    ESC_BIND = escBind;
}

void LuminSOCD::loadConfiguration() {
    std::ifstream config_file(CONFIG_NAME);
    if (!config_file) {
        configureKeyBindings(WASD, DEFAULT_DISABLE_BIND, Key::ESC);
        saveConfiguration(WASD, DEFAULT_DISABLE_BIND, Key::ESC);
        return;
    }

    std::array<Key*, 6> keys = { &CUSTOM_BINDS[0], &CUSTOM_BINDS[1], &CUSTOM_BINDS[2], &CUSTOM_BINDS[3], &DISABLE_BIND, &ESC_BIND };
    for (auto& key : keys) {
        unsigned int value;
        config_file >> std::hex >> value;
        *key = static_cast<Key>(value);
    }

    std::string line;
    size_t i = 0;
    while (std::getline(config_file, line) && i < whitelist_max_length && !line.empty()) {
        programs_whitelist[i++] = line;
    }
}

void LuminSOCD::saveConfiguration(const std::array<Key, 4>& bindings, const Key disableBind, const Key escBind) {
    std::ofstream config_file(CONFIG_NAME);
    if (!config_file) {
        std::cerr << "Couldn't open the config file\n";
        return;
    }

    auto write_key = [&](Key key) {
        config_file << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(key) << '\n';
        };

    std::for_each(bindings.begin(), bindings.end(), write_key);
    write_key(disableBind);
    write_key(escBind);

    for (const auto& program : programs_whitelist) {
        if (program.empty()) break;
        config_file << program << '\n';
    }
}

std::optional<Key> LuminSOCD::getOppositeKey(const Key key) {
    for (const auto& [dir1, dir2] : opposites) {
        if (key == CUSTOM_BINDS[static_cast<int>(dir1)])
            return CUSTOM_BINDS[static_cast<int>(dir2)];
        if (key == CUSTOM_BINDS[static_cast<int>(dir2)])
            return CUSTOM_BINDS[static_cast<int>(dir1)];
    }

    return std::nullopt;
}

std::optional<int> LuminSOCD::getKeyIndex(const Key key) {
    for (int i = 0; i < 4; ++i) {
        if (key == CUSTOM_BINDS[i]) return i;
    }
    return std::nullopt;
}

LRESULT CALLBACK LuminSOCD::handleKeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION || reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam)->flags & LLKHF_INJECTED) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    const auto& kbd = *reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    const auto key = static_cast<Key>(kbd.vkCode);
    const bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

    auto sendKey = [](Key k, bool up) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(static_cast<UINT>(k), MAPVK_VK_TO_VSC_EX));
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (up ? KEYEVENTF_KEYUP : 0);
        SendInput(1, &input, sizeof(INPUT));
        };

    if (key == DISABLE_BIND) {
        disableKeyPressed = isKeyDown ? IS_DOWN : IS_UP;
    }
    else if (key == ESC_BIND) {
        ESC_PRESSED = isKeyDown ? IS_DOWN : IS_UP;
        sendKey(Key::ESC, !isKeyDown);
    }
    else if (auto opposing = getOppositeKey(key)) {
        const auto [index, opposing_index] = std::make_pair(getKeyIndex(key), getKeyIndex(*opposing));
        real[*index] = virtual_keys[*index] = isKeyDown ? IS_DOWN : IS_UP;

        if (disableKeyPressed == IS_UP) {
            if (isKeyDown && real[*opposing_index] == IS_DOWN && virtual_keys[*opposing_index] == IS_DOWN) {
                sendKey(*opposing, true);
                virtual_keys[*opposing_index] = IS_UP;
            }
            else if (!isKeyDown && real[*opposing_index] == IS_DOWN) {
                sendKey(*opposing, false);
                virtual_keys[*opposing_index] = IS_DOWN;
            }
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void LuminSOCD::installKeyboardHook(const HINSTANCE instance) {
    if (hook_is_installed) return;

    std::cout << "Attempting to set keyboard hook...\n";
    kbhook = SetWindowsHookEx(WH_KEYBOARD_LL, handleKeyboardEvent, instance, 0);
    hook_is_installed = (kbhook != nullptr);
    std::cout << "Keyboard hook " << (hook_is_installed ? "successfully installed" : "installation failed") << '\n';
}

void LuminSOCD::removeKeyboardHook() {
    if (!hook_is_installed) return;

    std::cout << "Unsetting keyboard hook...\n";
    UnhookWindowsHookEx(kbhook);
    std::fill(real.begin(), real.end(), IS_UP);
    std::fill(virtual_keys.begin(), virtual_keys.end(), IS_UP);
    hook_is_installed = false;
    std::cout << "Keyboard hook unset. Hook status: uninstalled\n";
}

void LuminSOCD::updateFocusedProgram() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (!pid) return;

    if (HANDLE hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
        char buffer[MAX_PATH];
        DWORD size = sizeof(buffer);
        if (QueryFullProcessImageNameA(hproc, 0, buffer, &size)) {
            focused_program = buffer;
            PathStripPathA(focused_program.data());
            std::cout << "Window activated: " << focused_program << '\n';
        }
        CloseHandle(hproc);
    }
    else if (GetLastError() != ERROR_ACCESS_DENIED) {
        // Handle other errors if needed
    }
}

void LuminSOCD::handleFocusChange(
    HWINEVENTHOOK hWinEventHook, DWORD event, HWND window,
    LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime
) {
    updateFocusedProgram();

    const auto hInstance = GetModuleHandle(nullptr);
    for (const auto& program : programs_whitelist) {
        if (program.empty()) break;
        if (focused_program == program) {
            installKeyboardHook(hInstance);
            return;
        }
    }
    removeKeyboardHook();
}