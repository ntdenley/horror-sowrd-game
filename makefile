CC = gcc
CFLAGS = -Wall -O2 -static-libgcc -static-libstdc++
INCLUDES = -I ./lib/include
LIBS = -L ./lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer
TARGET = game.exe
SOURCE = src/game.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -o $(TARGET) $(LIBS)

clean:
	del $(TARGET)

dist: $(TARGET)
	if not exist dist mkdir dist
	copy $(TARGET) dist\
	copy lib\SDL2.dll dist\
	copy lib\SDL2_image.dll dist\
	copy lib\SDL2_mixer.dll dist\
	copy lib\libpng16-16.dll dist\
	copy lib\zlib1.dll dist\
	if exist src\audio mkdir dist\audio
	if exist src\audio copy src\audio\*.* dist\audio\
	if exist src\images mkdir dist\images
	if exist src\images copy src\images\*.* dist\images\

# Setup target to copy libraries from system
setup:
	if not exist lib mkdir lib
	if not exist lib\include mkdir lib\include
	if not exist lib\include\SDL2 mkdir lib\include\SDL2
	copy C:\msys64\ucrt64\include\SDL2\*.h lib\include\SDL2\
	copy C:\msys64\ucrt64\lib\libSDL2*.a lib\
	copy C:\msys64\ucrt64\lib\libmingw32.a lib\
	copy C:\msys64\ucrt64\bin\SDL2.dll lib\
	copy C:\msys64\ucrt64\bin\SDL2_image.dll lib\
	copy C:\msys64\ucrt64\bin\SDL2_mixer.dll lib\
	copy C:\msys64\ucrt64\bin\libpng16-16.dll lib\
	copy C:\msys64\ucrt64\bin\zlib1.dll lib\