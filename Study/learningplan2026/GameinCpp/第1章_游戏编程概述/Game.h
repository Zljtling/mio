#ifndef _GAME_H_
#define _GAME_H_

#include <SDL3/SDL.h>
#include <vector>

struct Vector2
{
	float x;
	float y;
};
struct Ball
{
	Vector2 mBallPos;
	Vector2 mBallVel;
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

	int mPaddleDir_P1;
	int mPaddleDir_P2;
	Vector2 mPaddlePos_P1;
	Vector2 mPaddlePos_P2;
	std::vector<Ball> ball;
};




#endif // !_GAME_H_
