/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- terminalInput.hpp

Abstract:
- This serves as an adapter between virtual key input from a user and the virtual terminal sequences that are
  typically emitted by an xterm-compatible console.

Author(s):
- Michael Niksa (MiNiksa) 30-Oct-2015
--*/

#include <functional>
#include "../../types/inc/IInputEvent.hpp"
#pragma once

namespace Microsoft::Console::VirtualTerminal
{
    class TerminalInput final
    {
    public:
        // We use a plain array instead of std::wstring because std::wstring can only hold 7
        // characters before heap allocating, but HandleKey() almost always ends up returning
        // >7 chars with Win32 mode enabled. It certainly makes no difference, but it's neat.
        //
        // Also, fast-ass HSTRINGs are annoying and must be null terminated.
        // The caller can do that if they have to... -.-
        using OutputType = std::array<wchar_t, 64>;
        
        struct MouseButtonState
        {
            bool isLeftButtonDown;
            bool isMiddleButtonDown;
            bool isRightButtonDown;
        };

        [[nodiscard]] int HandleKey(OutputType& out, const INPUT_RECORD& pInEvent);
        [[nodiscard]] int HandleFocus(OutputType& out, bool focused) const noexcept;
        [[nodiscard]] int HandleMouse( OutputType& out, til::point position, unsigned int button, short modifierKeyState, short delta, MouseButtonState state) noexcept;

        enum class Mode : size_t
        {
            LineFeed,
            Ansi,
            AutoRepeat,
            Keypad,
            CursorKey,
            BackarrowKey,
            Win32,

            Utf8MouseEncoding,
            SgrMouseEncoding,

            DefaultMouseTracking,
            ButtonEventMouseTracking,
            AnyEventMouseTracking,

            FocusEvent,

            AlternateScroll
        };

        void SetInputMode(Mode mode, bool enabled) noexcept;
        bool GetInputMode(Mode mode) const noexcept;
        void ResetInputModes() noexcept;
        void ForceDisableWin32InputMode(bool win32InputMode) noexcept;

#pragma region MouseInput
        // These methods are defined in mouseInput.cpp

        bool IsTrackingMouseInput() const noexcept;
        bool ShouldSendAlternateScroll(unsigned int button, short delta) const noexcept;
#pragma endregion

#pragma region MouseInputState Management
        // These methods are defined in mouseInputState.cpp
        void UseAlternateScreenBuffer() noexcept;
        void UseMainScreenBuffer() noexcept;
#pragma endregion

    private:
        // storage location for the leading surrogate of a utf-16 surrogate pair
        std::optional<wchar_t> _leadingSurrogate;

        std::optional<WORD> _lastVirtualKeyCode;

        til::enumset<Mode> _inputMode{ Mode::Ansi, Mode::AutoRepeat };
        bool _forceDisableWin32InputMode{ false };

        [[nodiscard]] int _SendChar(OutputType& out, wchar_t ch) noexcept;
        static [[nodiscard]] int _SendInputSequence(OutputType& out, const std::wstring_view& sequence) noexcept;
        static [[nodiscard]] int _SendEscapedInputSequence(OutputType& out, wchar_t wch) noexcept;
        static [[nodiscard]] int _GenerateWin32KeySequence(OutputType& out, const KEY_EVENT_RECORD& key) noexcept;
        static [[nodiscard]] int _searchWithModifier(OutputType& out, const KEY_EVENT_RECORD& keyEvent) noexcept;

#pragma region MouseInputState Management
        // These methods are defined in mouseInputState.cpp
        struct MouseInputState
        {
            bool inAlternateBuffer{ false };
            til::point lastPos{ -1, -1 };
            unsigned int lastButton{ 0 };
            int accumulatedDelta{ 0 };
        };

        MouseInputState _mouseInputState;
#pragma endregion

#pragma region MouseInput
        static [[nodiscard]] int _GenerateDefaultSequence(OutputType& out, til::point position, unsigned int button, bool isHover, short modifierKeyState, short delta);
        static [[nodiscard]] int _GenerateUtf8Sequence(OutputType& out, til::point position, unsigned int button, bool isHover, short modifierKeyState, short delta);
        static [[nodiscard]] int _GenerateSGRSequence(OutputType& out, til::point position, unsigned int button, bool isDown, bool isHover, short modifierKeyState, short delta);

        [[nodiscard]] int _SendAlternateScroll(OutputType& out, short delta) const noexcept;

        static constexpr unsigned int s_GetPressedButton(MouseButtonState state) noexcept;
#pragma endregion
    };
}
