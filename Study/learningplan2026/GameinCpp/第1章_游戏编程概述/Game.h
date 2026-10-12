#ifndef _GAME_H_
#define _GAME_H_

#include <SDL3/SDL.h>

struct Vector2
{
	float x;
	float y;
};

class Game
{
public:
	Game();
	//初始化游戏
	bool Initialize();
	//开始游戏直至结束
	void RunLoop();
	//关闭游戏
	void Shutdown();

private:
	//游戏循环辅助函数
	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	//SDL创建窗口
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	Uint32 mTicksCount;
	//游戏循环判断
	bool mIsRunning;

	int mPaddleDir;
	Vector2 mPaddlePos;
	Vector2 mBallPos;
	Vector2 mBallVel;
};




#endif // !_GAME_H_
