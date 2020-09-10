# OWN3D Pro OBS Plugin

With [OWN3D Pro](https://own3d.pro/) it’s easy to change Overlays and Alerts within a couple of seconds. The only thing you have to do is to choose your desired design and click install. Ready set go! [Join now for FREE!](https://own3d.pro/dashboard)

![OWN3D Pro OBS Plugin](https://assets.cdn.own3d.tv/production/uploads/chrome_2020-09-10_13-57-31.png)

This application currently only supports 64-bit Windows.

## Compiling

### Requirements
* CMake 3.16 or newer
* Visual Studio 2019 (Community works fine)* 
* An internet connection
* Optional: LLVM for clang-format integration
* Optional: Windows 10 SDK for 'signtool.exe'
* Optional: InnoSetup for the installer

### Steps to build for release
1. Open CMake (GUI).
2. Set Source directory to this directory.
3. Set Build directory to `<this directory>/build/flux`.
4. Click Configure.
5. Change OBS_DOWNLOAD to have a checkbox.
6. Change CMAKE_INSTALL_PREFIX to `<this directory>/build/install`.
6. Click Generate.
7. Click Open Project, which opens Visual Studio.
8. Change the build type to RelWithDebInfo.
9. Right click on 'INSTALL' and click 'Build'.
10. Your completed build is now available in `<this directory>/build/install`, all further steps are optional.
11. Go to `<this directory>/build/flux`.
12. Right click on `installer.iss` and select 'Compile'.
13. Once InnoSetup is done, an installer file is available in the same directory.
