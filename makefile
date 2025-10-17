all:
	g++ src/game.c -o main -I /ucrt64/include -L /ucrt64/lib -lSDL2 -lSDL2_image -lSDL2_mixer