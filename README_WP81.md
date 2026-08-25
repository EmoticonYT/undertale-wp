# UNDERTALE for Windows Phone 8.1 (Powered by Butterscotch)

This is a native Windows Phone 8.1 port of **UNDERTALE** (v1.08) using the **Butterscotch** open-source GameMaker: Studio runner.

---

## Features

* **Native ARM32 Windows Phone 8.1 AppX**: Runs directly on Windows Phone 8.1 (Lumia 520, 630, 920, 925, 930, 1020, 1520, etc.).
* **On-Screen Touch Controls**:
  * **Left side**: Virtual 8-way D-Pad for moving Frisk and dodging in battle.
  * **Right side**: Virtual buttons for **[Z]** (Confirm/Interact), **[X]** (Cancel/Run), and **[C]** (Menu/Inventory).
  * **Hardware Back Button**: Mapped to Cancel / Escape.
* **Low Memory Optimization**: Configured with `DATAWINLOADTYPE_LOAD_PER_CHUNK`, `lazyRooms`, `lazyTextures`, and `lazyAudio` to run smoothly on 512MB RAM devices.
* **Save State Persistence**: Automatically persists `undertale.ini`, `file0`, `file9`, etc., in the app's isolated `LocalFolder`.

---

## Project Structure

* `src/winphone/`:
  * `main.cpp`: CoreApplication / IFrameworkView entry point for WinRT.
  * `touch_overlay.h` & `touch_overlay.c`: Multi-touch controller overlay.
  * `log.c`: `OutputDebugStringA` logger.
* `winphone/`:
  * `Package.appxmanifest`: Windows Phone 8.1 AppX configuration.
  * `Assets/`: App tile icons (150x150, 44x44, 71x71, 50x50) and splash screens.
  * `Butterscotch.WinPhone.sln` / `.vcxproj`: Visual Studio solution.
* `UndertaleData/`: Game assets (`data.win`, `.ogg` audio files, `options.ini`).
* `tools/package_wp81.py`: Automated AppX packager.
* `.github/workflows/build-wp81.yml`: Automated CI build on GitHub Actions.

---

## How to Build

### Option 1: Automated GitHub Actions Build (Recommended from Mac)
1. Push this repository to GitHub.
2. Go to the **Actions** tab in your repository.
3. The **Build Windows Phone 8.1 Package** workflow will compile the ARM binary, bundle the Undertale data, and generate `Undertale_WP8.1.appx` in the workflow run artifacts.
4. Download the `.appx` artifact to your computer or phone.

### Option 2: Visual Studio (Windows PC or VM)
1. Open `winphone/Butterscotch.WinPhone.sln` in **Visual Studio 2013** or **Visual Studio 2015**.
2. Select **Release** configuration and **ARM** platform (or **Win32** for the Windows Phone Emulator).
3. Build the solution (`Ctrl+Shift+B`).
4. Run `python tools/package_wp81.py` to package the `.appx`.

---

## How to Sideload on Windows Phone 8.1

### Prerequisites
* A Windows Phone 8.1 device that is **Developer Unlocked** or **Interop Unlocked** (e.g. via Windows Phone Developer Registration tool or WPInternals).

### Deployment Methods:
1. **Windows Phone Application Deployment (VS / Windows Phone 8.1 SDK)**:
   * Connect your phone via USB.
   * Open *Windows Phone Application Deployment*.
   * Select **Target: Device**, browse for `Undertale_WP8.1.appx`, and click **Deploy**.
2. **WPV XAP / AppX Deployer**:
   * Connect phone via USB, launch WPV XAP Deployer, select `Undertale_WP8.1.appx`, and click Deploy.
3. **SD Card / Phone Storage Sideloading (Interop Unlocked)**:
   * Copy `Undertale_WP8.1.appx` to the phone's SD Card or Documents folder.
   * Open the **Files** app on your phone, tap the `.appx` file, and confirm installation.