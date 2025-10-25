@echo off
echo Copying SDL2 dependencies to lib folder...

REM Create lib directory if it doesn't exist
if not exist lib mkdir lib

REM Copy DLLs from ucrt64/bin
copy C:\msys64\ucrt64\bin\SDL2.dll lib\
copy C:\msys64\ucrt64\bin\SDL2_image.dll lib\
copy C:\msys64\ucrt64\bin\SDL2_mixer.dll lib\
copy C:\msys64\ucrt64\bin\libpng16-16.dll lib\
copy C:\msys64\ucrt64\bin\libjpeg-8.dll lib\
copy C:\msys64\ucrt64\bin\libwebp-7.dll lib\
copy C:\msys64\ucrt64\bin\libwebpdemux-2.dll lib\
copy C:\msys64\ucrt64\bin\libtiff-6.dll lib\
copy C:\msys64\ucrt64\bin\libavif-16.dll lib\
copy C:\msys64\ucrt64\bin\libjxl.dll lib\
copy C:\msys64\ucrt64\bin\libmpg123-0.dll lib\
copy C:\msys64\ucrt64\bin\libopusfile-0.dll lib\

REM Copy any additional dependencies that might be needed
copy C:\msys64\ucrt64\bin\zlib1.dll lib\
copy C:\msys64\ucrt64\bin\libvorbis-0.dll lib\
copy C:\msys64\ucrt64\bin\libvorbisfile-3.dll lib\
copy C:\msys64\ucrt64\bin\libogg-0.dll lib\
copy C:\msys64\ucrt64\bin\libgcc_s_seh-1.dll lib\
copy C:\msys64\ucrt64\bin\libwinpthread-1.dll lib\
copy C:\msys64\ucrt64\bin\libstdc++-6.dll lib\

echo Done! All dependencies copied to lib folder.
echo Note: System DLLs (KERNEL32.dll, USER32.dll, etc.) are already on Windows systems.
pause