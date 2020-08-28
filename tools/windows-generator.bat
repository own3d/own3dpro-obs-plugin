pushd ..
cmake -H. -Bbuild/64 -G "Visual Studio 16 2019" -A"x64" -DOBS_DOWNLOAD=TRUE -DCMAKE_INSTALL_PREFIX=build/install
cmake -H. -Bbuild/32 -G "Visual Studio 16 2019" -A"Win32" -DOBS_DOWNLOAD=TRUE -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build/64 --config RelWithDebInfo --target INSTALL
cmake --build build/32 --config RelWithDebInfo --target INSTALL
cmake --build build/64 --config RelWithDebInfo --target PACKAGE_ZIP
cmake --build build/64 --config RelWithDebInfo --target PACKAGE_7Z
build\64\installer.iss
popd
