@echo off
echo Packaging Horror Sword Game for distribution...

REM Create distribution directory
if exist "dist" rmdir /s /q "dist"
mkdir "dist"

REM Copy the main executable
copy "main.exe" "dist\"

REM Copy game assets
xcopy "src\audio" "dist\audio\" /E /I /Y
xcopy "src\images" "dist\images\" /E /I /Y

REM Copy required SDL2 DLLs from MinGW installation
copy "C:\msys64\ucrt64\bin\SDL2.dll" "dist\"
copy "C:\msys64\ucrt64\bin\SDL2_image.dll" "dist\"
copy "C:\msys64\ucrt64\bin\SDL2_mixer.dll" "dist\"

REM Copy additional dependencies that SDL2 libraries might need
copy "C:\msys64\ucrt64\bin\libgcc_s_seh-1.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libstdc++-6.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libwinpthread-1.dll" "dist\" 2>nul

REM Copy image format support libraries
copy "C:\msys64\ucrt64\bin\libpng16-16.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libjpeg-8.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libwebp-7.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libtiff-6.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\zlib1.dll" "dist\" 2>nul

REM Copy audio format support libraries
copy "C:\msys64\ucrt64\bin\libmpg123-0.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libvorbis-0.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libvorbisfile-3.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libogg-0.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libopusfile-0.dll" "dist\" 2>nul
copy "C:\msys64\ucrt64\bin\libmodplug-1.dll" "dist\" 2>nul

echo.
echo Distribution package created in 'dist' folder!
echo You can now zip the 'dist' folder and share it with others.
echo.
pause
