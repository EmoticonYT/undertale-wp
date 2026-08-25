#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <wrl/client.h>
#include <ppltasks.h>
#include <string>
#include <codecvt>
#include <locale>

#include "loop.h"
#include "platformdefs.h"
#include "touch_overlay.h"
#include "runner_keyboard.h"
#include "log.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::Phone::UI::Input;

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

ref class ButterscotchApp sealed : public IFrameworkView {
private:
    bool m_windowClosed;
    bool m_windowVisible;
    std::string m_installedPath;
    std::string m_localFolderPath;
    std::string m_dataWinPath;

public:
    ButterscotchApp() : m_windowClosed(false), m_windowVisible(true) {}

    virtual void Initialize(CoreApplicationView^ applicationView) {
        applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &ButterscotchApp::OnActivated);
        CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &ButterscotchApp::OnSuspending);
        CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &ButterscotchApp::OnResuming);

        // Hardware Back Button on Windows Phone
        HardwareButtons::BackPressed += ref new EventHandler<BackPressedEventArgs^>(this, &ButterscotchApp::OnBackPressed);
    }

    virtual void SetWindow(CoreWindow^ window) {
        window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &ButterscotchApp::OnWindowSizeChanged);
        window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &ButterscotchApp::OnVisibilityChanged);
        window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &ButterscotchApp::OnWindowClosed);

        // Multi-touch events
        window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ButterscotchApp::OnPointerPressed);
        window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ButterscotchApp::OnPointerMoved);
        window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ButterscotchApp::OnPointerReleased);
        window->PointerCaptureLost += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ButterscotchApp::OnPointerCancelled);

        // Initialize touch overlay dimensions
        Rect bounds = window->Bounds;
        TouchOverlay_Init((int)bounds.Width, (int)bounds.Height);
    }

    virtual void Load(Platform::String^ entryPoint) {
        // Resolve package installed directory and save directory
        StorageFolder^ installFolder = Package::Current->InstalledLocation;
        m_installedPath = PlatformStringToUtf8(installFolder->Path);

        StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
        m_localFolderPath = PlatformStringToUtf8(localFolder->Path);

        // Check if data.win is in UndertaleData/ or at root
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
        args.osType = OS_WINPHONE;
        args.profilerFramesBetween = 0;
        // Load per chunk to stay within memory limits on 512MB Windows Phones
        args.loadType = DATAWINLOADTYPE_LOAD_PER_CHUNK;
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

    void OnBackPressed(Platform::Object^ sender, BackPressedEventArgs^ args) {
        // Intercept back button to act as Escape / Cancel in Undertale
        args->Handled = true;
    }

    void OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args) {
        TouchOverlay_UpdateScreenSize((int)args->Size.Width, (int)args->Size.Height);
    }

    void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args) {
        m_windowVisible = args->Visible;
    }

    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args) {
        m_windowClosed = true;
    }

    void OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args) {
        PointerPoint^ pt = args->CurrentPoint;
        TouchOverlay_OnPointerDown(pt->PointerId, pt->Position.X, pt->Position.Y, nullptr);
    }

    void OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args) {
        PointerPoint^ pt = args->CurrentPoint;
        TouchOverlay_OnPointerMove(pt->PointerId, pt->Position.X, pt->Position.Y, nullptr);
    }

    void OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args) {
        PointerPoint^ pt = args->CurrentPoint;
        TouchOverlay_OnPointerUp(pt->PointerId, pt->Position.X, pt->Position.Y, nullptr);
    }

    void OnPointerCancelled(CoreWindow^ sender, PointerEventArgs^ args) {
        PointerPoint^ pt = args->CurrentPoint;
        TouchOverlay_OnPointerCancel(pt->PointerId, nullptr);
    }
};

ref class ButterscotchAppSource sealed : IFrameworkViewSource {
public:
    virtual IFrameworkView^ CreateView() {
        return ref new ButterscotchApp();
    }
};

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^ args) {
    auto appSource = ref new ButterscotchAppSource();
    CoreApplication::Run(appSource);
    return 0;
}
