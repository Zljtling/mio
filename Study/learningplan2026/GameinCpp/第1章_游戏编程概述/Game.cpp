#include "Game.h"

const int thickness = 15;		//墙壁厚度
const float paddleH = 100.0f;	//球拍高度

Game::Game()
	:mWindow(nullptr)
	, mRenderer(nullptr)
	, mTicksCount(0)
	, mIsRunning(true)
	, mPaddleDir(0)
{

}

bool Game::Initialize()
{
	//游戏初始化
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}
	mWindow = SDL_CreateWindow("Game Programming in C++ (Chapter 1)", 1024, 768, 0);
	if (!mWindow)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}
	mRenderer = SDL_CreateRenderer(mWindow, NULL);
	if (!mRenderer)
	{
		SDL_Log("Failed to create renderer : %s", SDL_GetError());
		return false;
	}
	//球拍和球位置及球速度设置
	mPaddlePos.x = 10.0f;
	mPaddlePos.y = 768.0f / 2.0f;
	mBallPos.x = 1024.0f / 2.0f;
	mBallPos.y = 768.0f / 2.0f;
	mBallVel.x = -200.0f;
	mBallVel.y = 235.0f;
	return true;
}

void Game::RunLoop()
{
	//游戏循环逻辑
	while (mIsRunning)
	{
		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
}

void Game::ProcessInput()
{
	//初始化接收器
	SDL_Event event;
	while (SDL_PollEvent(&event))
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			mIsRunning = false;
			break;
		}
	const bool* state = SDL_GetKeyboardState(NULL);
	//退出按键
	if (state[SDL_SCANCODE_ESCAPE])
		mIsRunning = false;
	//球拍按键
	mPaddleDir = 0;
	if (state[SDL_SCANCODE_W])
		mPaddleDir -= 1;
	if (state[SDL_SCANCODE_S])
		mPaddleDir += 1;
}

void Game::UpdateGame()
{
	//处理帧速率
	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;
	mTicksCount = SDL_GetTicks();
	//球拍的移动和边界检测
	if (mPaddleDir != 0)
	{
		mPaddlePos.y += mPaddleDir * 300.0f * deltaTime;
		if (mPaddlePos.y < (paddleH / 2.0f + thickness))
			mPaddlePos.y = paddleH / 2.0f + thickness;
		else if (mPaddlePos.y > (768.0f - paddleH / 2.0f - thickness))
			mPaddlePos.y = 768.0f - paddleH / 2.0f - thickness;
	}
	//球的移动速率
	mBallPos.x += mBallVel.x * deltaTime;
	mBallPos.y += mBallVel.y * deltaTime;

	//计算球拍中心与球中心的高度差
	float diff = mPaddlePos.y - mBallPos.y;
	diff = (diff > 0.0f) ? diff : -diff;
	if (diff <= paddleH / 2.0f && mBallPos.x <= 25.0f && mBallPos.x >= 20.0f && mBallVel.x < 0.0f)
		mBallVel.x *= -1.0f;
	else if (mBallPos.x <= 0.0f)
		mIsRunning = false;
	else if (mBallPos.x >= (1024.0f - thickness) && mBallVel.x > 0.0f)
		mBallVel.x *= -1.0f;
	//处理球碰撞上下墙反弹逻辑
	if (mBallPos.y <= thickness && mBallVel.y < 0.0f)
		mBallVel.y *= -1;
	else if (mBallPos.y >= (768 - thickness) && mBallVel.y > 0.0f)
		mBallVel.y *= -1;
}

void Game::GenerateOutput()
{
	//设定背景
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 255, 255);
	SDL_RenderClear(mRenderer);
	//绘制墙
	SDL_SetRenderDrawColor(mRenderer, 255, 255, 255, 255);
	SDL_FRect wall{
		0,
		0,
		1024,
		thickness
	};
	SDL_RenderFillRect(mRenderer, &wall);
	wall.y = 768 - thickness;
	SDL_RenderFillRect(mRenderer, &wall);
	wall.x = 1024 - thickness;
	wall.y = 0;
	wall.w = thickness;
	wall.h = 1024;
	SDL_RenderFillRect(mRenderer, &wall);
	//绘制球拍
	SDL_FRect paddle{
		static_cast<int>(mPaddlePos.x),
		static_cast<int>(mPaddlePos.y - paddleH / 2),
		thickness,
		static_cast<int>(paddleH)
	};
	SDL_RenderFillRect(mRenderer, &paddle);
	//绘制球
	SDL_FRect ball{
		static_cast<int>(mBallPos.x - thickness / 2),
		static_cast<int>(mBallPos.y - thickness / 2),
		thickness,
		thickness
	};
	SDL_RenderFillRect(mRenderer, &ball);
	//绘制图像
	SDL_RenderPresent(mRenderer);
}

void Game::Shutdown()
{	
	SDL_DestroyRenderer(mRenderer);	//删除渲染器
	SDL_DestroyWindow(mWindow);		//删除窗口
	SDL_Quit();						//推出游戏
}
