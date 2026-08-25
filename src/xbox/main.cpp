#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <wrl/client.h>
#include <ppltasks.h>
#include <string>
#include <vector>

#include "loop.h"
#include "platformdefs.h"
#include "runner_keyboard.h"
#include "log.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::Gaming::Input;

static std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

static std::string PlatformStringToUtf8(Platform::String^ ps) {
    if (ps == nullptr) return std::string();
    return WStringToUtf8(ps->Data());
}

// Controller state tracking
struct ControllerMappingState {
    bool up, down, left, right;
    bool btnA, btnB, btnMenu;
} g_ctrlState = {0};

void PollXboxController(RunnerKeyboardState* keyboard) {
    if (!keyboard) return;

    auto gamepads = Gamepad::Gamepads;
    if (gamepads->Size == 0) return;

    Gamepad^ pad = gamepads->GetAt(0);
    GamepadReading reading = pad->GetCurrentReading();

    const float DEADZONE = 0.35f;

    // Stick: Left analog stick -> Arrow keys
    bool stickUp    = (reading.LeftThumbstickY > DEADZONE);
    bool stickDown  = (reading.LeftThumbstickY < -DEADZONE);
    bool stickLeft  = (reading.LeftThumbstickX < -DEADZONE);
    bool stickRight = (reading.LeftThumbstickX > DEADZONE);

    // D-Pad -> Arrow keys
    bool dpadUp    = (reading.Buttons & GamepadButtons::DPadUp) != GamepadButtons::None;
    bool dpadDown  = (reading.Buttons & GamepadButtons::DPadDown) != GamepadButtons::None;
    bool dpadLeft  = (reading.Buttons & GamepadButtons::DPadLeft) != GamepadButtons::None;
    bool dpadRight = (reading.Buttons & GamepadButtons::DPadRight) != GamepadButtons::None;

    bool isUp    = stickUp || dpadUp;
    bool isDown  = stickDown || dpadDown;
    bool isLeft  = stickLeft || dpadLeft;
    bool isRight = stickRight || dpadRight;

    // Button A -> Enter / Z
    bool isBtnA = (reading.Buttons & GamepadButtons::A) != GamepadButtons::None;

    // Button B -> X / Shift
    bool isBtnB = (reading.Buttons & GamepadButtons::B) != GamepadButtons::None;

    // Menu (Start) -> C / Control
    bool isBtnMenu = (reading.Buttons & GamepadButtons::Menu) != GamepadButtons::None;

    // Helper macro to dispatch key state changes
    #define UPDATE_KEY(curr, prev, vk1, vk2) do {         if ((curr) != (prev)) {             if (curr) {                 RunnerKeyboard_onKeyDown(keyboard, vk1);                 if (vk2 != 0) RunnerKeyboard_onKeyDown(keyboard, vk2);             } else {                 RunnerKeyboard_onKeyUp(keyboard, vk1);                 if (vk2 != 0) RunnerKeyboard_onKeyUp(keyboard, vk2);             }         }     } while(0)

    UPDATE_KEY(isUp,    g_ctrlState.up,    VK_UP,     0);
    UPDATE_KEY(isDown,  g_ctrlState.down,  VK_DOWN,   0);
    UPDATE_KEY(isLeft,  g_ctrlState.left,  VK_LEFT,   0);
    UPDATE_KEY(isRight, g_ctrlState.right, VK_RIGHT,  0);

    // A -> Enter (VK_RETURN) and 'Z'
    UPDATE_KEY(isBtnA,  g_ctrlState.btnA,  VK_RETURN, 'Z');

    // B -> 'X' and VK_SHIFT
    UPDATE_KEY(isBtnB,  g_ctrlState.btnB,  'X',       VK_SHIFT);

    // Menu -> 'C' and VK_CONTROL
    UPDATE_KEY(isBtnMenu, g_ctrlState.btnMenu, 'C',   VK_CONTROL);

    #undef UPDATE_KEY

    g_ctrlState.up = isUp;
    g_ctrlState.down = isDown;
    g_ctrlState.left = isLeft;
    g_ctrlState.right = isRight;
    g_ctrlState.btnA = isBtnA;
    g_ctrlState.btnB = isBtnB;
    g_ctrlState.btnMenu = isBtnMenu;
}

ref class XboxApp sealed : public IFrameworkView {
private:
    bool m_windowClosed;
    bool m_windowVisible;
    std::string m_installedPath;
    std::string m_localFolderPath;
    std::string m_dataWinPath;

public:
    XboxApp() : m_windowClosed(false), m_windowVisible(true) {}

    virtual void Initialize(CoreApplicationView^ applicationView) {
        applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &XboxApp::OnActivated);
        CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &XboxApp::OnSuspending);
        CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &XboxApp::OnResuming);
    }

    virtual void SetWindow(CoreWindow^ window) {
        window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &XboxApp::OnVisibilityChanged);
        window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &XboxApp::OnWindowClosed);
    }

    virtual void Load(Platform::String^ entryPoint) {
        StorageFolder^ installFolder = Package::Current->InstalledLocation;
        m_installedPath = PlatformStringToUtf8(installFolder->Path);

        StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
        m_localFolderPath = PlatformStringToUtf8(localFolder->Path);

        std::string candidate1 = m_installedPath + "\\UndertaleData\\data.win";
        std::string candidate2 = m_installedPath + "\\data.win";

        DWORD attr1 = GetFileAttributesA(candidate1.c_str());
        if (attr1 != INVALID_FILE_ATTRIBUTES && !(attr1 & FILE_ATTRIBUTE_DIRECTORY)) {
            m_dataWinPath = candidate1;
        } else {
            m_dataWinPath = candidate2;
        }
    }

    virtual void Run() {
        CommandLineArgs args = {0};

        args.exitAtFrame = -1;
#ifdef ENABLE_VM_TRACING
        args.traceBytecodeAfterFrame = 0;
#endif
        args.speedMultiplier = 1.0;
        args.fastForwardSpeed = 0.0;
        args.osType = OS_UWP;
        args.profilerFramesBetween = 0;
        args.loadType = DATAWINLOADTYPE_LOAD_ALL;
        args.lazyRooms = true;
        args.lazyTextures = true;
        args.lazyAudio = true;
#if defined(ENABLE_MODERN_GL)
        args.renderer = MODERN_GL;
#elif defined(ENABLE_LEGACY_GL)
        args.renderer = LEGACY_GL;
#else
        args.renderer = SOFTWARE;
#endif
        args.dataWinPath = m_dataWinPath.c_str();
        args.saveFolder = m_localFolderPath.c_str();

        loop(args, "Butterscotch");
        freeCommandLineArgs(&args);
    }

    virtual void Uninitialize() {}

private:
    void OnActivated(CoreApplicationView^ sender, IActivatedEventArgs^ args) {
        CoreWindow::GetForCurrentThread()->Activate();
    }

    void OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args) {
        SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
        deferral->Complete();
    }

    void OnResuming(Platform::Object^ sender, Platform::Object^ args) {}

    void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args) {
        m_windowVisible = args->Visible;
    }

    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args) {
        m_windowClosed = true;
    }
};

ref class XboxAppSource sealed : IFrameworkViewSource {
public:
    virtual IFrameworkView^ CreateView() {
        return ref new XboxApp();
    }
};

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^ args) {
    auto appSource = ref new XboxAppSource();
    CoreApplication::Run(appSource);
    return 0;
}
