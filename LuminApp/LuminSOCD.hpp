#pragma once
#ifndef LUMINSOCD_HPP
#define LUMINSOCD_HPP

#include <array>
#include <string>
#include <vector>
#include <optional>
#include <windows.h>

/**
 * @enum Key
 * @brief Represents keyboard keys with their corresponding virtual key codes.
 */
enum class Key : unsigned int {
    ESC = 0x01,
    W = 0x57,
    A = 0x41,
    S = 0x53,
    D = 0x44,
    E = 0x45,
    UP = VK_UP,
    LEFT = VK_LEFT,
    DOWN = VK_DOWN,
    RIGHT = VK_RIGHT
};

/**
 * @enum Direction
 * @brief Represents cardinal directions.
 */
enum class Direction {
    Left = 0,
    Right,
    Up,
    Down
};

inline HWND main_window;

constexpr int IS_DOWN = 1;
constexpr int IS_UP = 0;
constexpr int whitelist_max_length = 200;

inline const auto CONFIG_NAME = "socd.conf";

inline std::string config_line;
inline std::string focused_program;
inline std::array<std::string, whitelist_max_length> programs_whitelist{};

inline HHOOK kbhook;
inline bool hook_is_installed = false;
inline bool listening_for_esc_bind = false;

inline std::array<int, 4> real{};
inline std::array<int, 4> virtual_keys{};

constexpr auto DEFAULT_DISABLE_BIND = Key::E;
inline Key DISABLE_BIND;
inline int disableKeyPressed;

inline auto ESC_BIND = Key::ESC;
inline int ESC_PRESSED;

static constexpr std::array WASD = { Key::A, Key::D, Key::W, Key::S };
static constexpr std::array ARROWS = { Key::LEFT, Key::RIGHT, Key::UP, Key::DOWN };
static const std::array<std::pair<Direction, Direction>, 2> opposites = {
        {{Direction::Left, Direction::Right},
         {Direction::Up, Direction::Down}}
};
inline std::array<Key, 4> CUSTOM_BINDS;

class LuminSOCD {
public:
    static void initializeWindow(HWND handle);

    /**
     * @brief Sets the current key bindings.
     * @param bindings Array of key bindings
     * @param disableBind Key used to disable SOCD cleaning
     * @param escBind Key used for ESC functionality
     */
    static void configureKeyBindings(const std::array<Key, 4>& bindings, Key disableBind, Key escBind);

    /**
     * @brief Writes the current settings to the configuration file.
     * @param bindings Array of key bindings
     * @param disableBind Key used to disable SOCD cleaning
     * @param escBind Key used for ESC functionality
     */
    static void saveConfiguration(const std::array<Key, 4>& bindings, Key disableBind, Key escBind);

    /**
     * @brief Reads settings from the configuration file.
     */
    static void loadConfiguration();

    /**
     * @brief Finds the opposing key for a given key.
     * @param key The key to find the opposite for
     * @return An optional containing the opposing key if found
     */
    static std::optional<Key> getOppositeKey(Key key);

    /**
     * @brief Finds the index of a given key in the bindings array.
     * @param key The key to find the index for
     * @return An optional containing the index if found
     */
    static std::optional<int> getKeyIndex(Key key);

    /**
     * @brief Low-level keyboard procedure for handling key events.
     * @param nCode The hook code
     * @param wParam The identifier of the keyboard message
     * @param lParam A pointer to a KBDLLHOOKSTRUCT structure
     * @return The hook procedure return value
     */
    static LRESULT CALLBACK handleKeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Sets the keyboard hook.
     * @param instance The handle to the application instance
     */
    static void installKeyboardHook(HINSTANCE instance);

    /**
    * @brief Unsets the keyboard hook.
    */
    static void removeKeyboardHook();

    /**
     * @brief Gets the name of the currently focused program.
     */
    static void updateFocusedProgram();

    /**
     * @brief Callback function to detect changes in the focused program.
     * @param hWinEventHook Handle to an event hook function
     * @param event The event that occurred
     * @param window Handle to the window that generated the event
     * @param idObject Identifier of the object that generated the event
     * @param idChild Identifier of the child object that generated the event
     * @param idEventThread Identifier of the thread that generated the event
     * @param dwmsEventTime The time that the event was generated
     */
    static void CALLBACK handleFocusChange(
        HWINEVENTHOOK hWinEventHook, DWORD event, HWND window,
        LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime
    );
};

#endif //LUMINSOCD_HPP
