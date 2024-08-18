#include "pch.h"
#include "MainWindow.xaml.h"
#include "LuminSOCD.hpp"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::UI::ViewManagement;

namespace winrt::LuminApp::implementation
{
    MainWindow::MainWindow() {
        auto windowNative{ this->m_inner.as<::IWindowNative>() };

        HWND hWnd{ 0 };
        windowNative->get_WindowHandle(&hWnd);
        // Window Styling
        setupWindow(hWnd);
        // Title
        Title(L"Lumin SOCD");
        ExtendsContentIntoTitleBar(true);
        // Init SOCD Stuff
        LuminSOCD::initializeWindow(hWnd);
    }

    void MainWindow::setupWindow(HWND hwnd) {
        // Disable resizing the window
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        SetWindowLong(hwnd, GWL_STYLE, style);
        // Resize to 250x325
        SetWindowPos(hwnd, NULL, 0, 0, 250, 325, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        
    }


    void MainWindow::OnKeybindingChanged(winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const& e)
    {
        if (sender == wasdRadio()) {
            LuminSOCD::configureKeyBindings(WASD, DEFAULT_DISABLE_BIND, ESC_BIND);
            LuminSOCD::saveConfiguration(WASD, DEFAULT_DISABLE_BIND, ESC_BIND);
        }
        else if (sender == arrowsRadio())
        {
            LuminSOCD::configureKeyBindings(ARROWS, DEFAULT_DISABLE_BIND, ESC_BIND);
            LuminSOCD::saveConfiguration(ARROWS, DEFAULT_DISABLE_BIND, ESC_BIND);
        }
        else if (sender == customRadio())
        {
            MessageBox(NULL, L"This is a work in progress!", L"MessageBox", NULL);
            
        }
    }
}
