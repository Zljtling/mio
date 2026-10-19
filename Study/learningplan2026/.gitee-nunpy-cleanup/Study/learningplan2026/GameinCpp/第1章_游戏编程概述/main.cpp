#include "Game.h"
#include <SDL3/SDL_main.h>

int main(int argc, char* argv[])
{
	Game game;
	bool success = game.Initialize();
	if (success)
	{
		game.RunLoop();
	}
	game.Shutdown();
	return 0;
}
