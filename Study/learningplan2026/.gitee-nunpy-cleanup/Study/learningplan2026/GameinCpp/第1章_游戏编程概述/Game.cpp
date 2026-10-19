#include "Game.h"

const int thickness = 15;		//墙壁厚度
const float paddleH = 100.0f;	//球拍高度

Game::Game()
	:mWindow(nullptr)
	, mRenderer(nullptr)
	, mTicksCount(0)
	, mIsRunning(true)
	, mPaddleDir_P1(0)
	, mPaddleDir_P2(0)
	, ball(2)
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
	mPaddlePos_P1.x = 10.0f;
	mPaddlePos_P1.y = 768.0f / 2.0f;
	mPaddlePos_P2.x = 1009.0f;
	mPaddlePos_P2.y = 768.0f / 2.0f;
	ball[0].mBallPos.x = 1024.0f / 2.0f;
	ball[0].mBallPos.y = 768.0f / 2.0f;
	ball[0].mBallVel.x = -200.0f;
	ball[0].mBallVel.y = 235.0f;
	ball[1].mBallPos.x = 1024.0f / 2.0f;
	ball[1].mBallPos.y = 768.0f / 2.0f;
	ball[1].mBallVel.x = 200.0f;
	ball[1].mBallVel.y = -235.0f;
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
	mPaddleDir_P1 = 0;
	if (state[SDL_SCANCODE_W])
		mPaddleDir_P1 -= 1;
	if (state[SDL_SCANCODE_S])
		mPaddleDir_P1 += 1;
	mPaddleDir_P2 = 0;
	if (state[SDL_SCANCODE_I])
		mPaddleDir_P2 -= 1;
	if (state[SDL_SCANCODE_K])
		mPaddleDir_P2 += 1;
}

void Game::UpdateGame()
{
	//处理帧速率
	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;
	mTicksCount = SDL_GetTicks();
	//球拍的移动和边界检测
	if (mPaddleDir_P1 != 0)
	{
		mPaddlePos_P1.y += mPaddleDir_P1 * 300.0f * deltaTime;
		if (mPaddlePos_P1.y < (paddleH / 2.0f + thickness))
			mPaddlePos_P1.y = paddleH / 2.0f + thickness;
		else if (mPaddlePos_P1.y > (768.0f - paddleH / 2.0f - thickness))
			mPaddlePos_P1.y = 768.0f - paddleH / 2.0f - thickness;
	}
	if (mPaddleDir_P2 != 0)
	{
		mPaddlePos_P2.y += mPaddleDir_P2 * 300.0f * deltaTime;
		if (mPaddlePos_P2.y < (paddleH / 2.0f + thickness))
			mPaddlePos_P2.y = paddleH / 2.0f + thickness;
		else if (mPaddlePos_P2.y > (768.0f - paddleH / 2.0f - thickness))
			mPaddlePos_P2.y = 768.0f - paddleH / 2.0f - thickness;
	}
	//球的移动速率
	ball[0].mBallPos.x += ball[0].mBallVel.x * deltaTime;
	ball[0].mBallPos.y += ball[0].mBallVel.y * deltaTime;
	ball[1].mBallPos.x += ball[1].mBallVel.x * deltaTime;
	ball[1].mBallPos.y += ball[1].mBallVel.y * deltaTime;
	//计算球拍中心与球中心的高度差
	float diff_P1_b0 = mPaddlePos_P1.y - ball[0].mBallPos.y;
	float diff_P2_b0 = mPaddlePos_P2.y - ball[0].mBallPos.y;
	diff_P1_b0 = (diff_P1_b0 > 0.0f) ? diff_P1_b0 : -diff_P1_b0;
	diff_P2_b0 = (diff_P2_b0 > 0.0f) ? diff_P2_b0 : -diff_P2_b0;
	if (diff_P1_b0 <= paddleH / 2.0f && ball[0].mBallPos.x <= 25.0f && ball[0].mBallPos.x >= 20.0f && ball[0].mBallVel.x < 0.0f)
		ball[0].mBallVel.x *= -1.0f;
	else if (ball[0].mBallPos.x <= 0.0f || ball[0].mBallPos.x >= 1024.0f)
		mIsRunning = false;
	else if (diff_P2_b0 <= paddleH / 2.0f && ball[0].mBallPos.x >= (1024.0f - thickness) && ball[0].mBallPos.x <= 1014.0f && ball[0].mBallVel.x > 0.0f)
		ball[0].mBallVel.x *= -1.0f;
	//处理球碰撞上下墙反弹逻辑
	if (ball[0].mBallPos.y <= thickness && ball[0].mBallVel.y < 0.0f)
		ball[0].mBallVel.y *= -1;
	else if (ball[0].mBallPos.y >= (768 - thickness) && ball[0].mBallVel.y > 0.0f)
		ball[0].mBallVel.y *= -1;

	float diff_P1_b1 = mPaddlePos_P1.y - ball[1].mBallPos.y;
	float diff_P2_b1 = mPaddlePos_P2.y - ball[1].mBallPos.y;
	diff_P1_b1 = (diff_P1_b1 > 0.0f) ? diff_P1_b1 : -diff_P1_b1;
	diff_P2_b1 = (diff_P2_b1 > 0.0f) ? diff_P2_b1 : -diff_P2_b1;
	if (diff_P1_b1 <= paddleH / 2.0f && ball[1].mBallPos.x <= 25.0f && ball[1].mBallPos.x >= 20.0f && ball[1].mBallVel.x < 0.0f)
		ball[1].mBallVel.x *= -1.0f;
	else if (ball[1].mBallPos.x <= 0.0f || ball[1].mBallPos.x >= 1024.0f)
		mIsRunning = false;
	else if (diff_P2_b1 <= paddleH / 2.0f && ball[1].mBallPos.x >= (1024.0f - thickness) && ball[1].mBallPos.x <= 1014.0f && ball[1].mBallVel.x > 0.0f)
		ball[1].mBallVel.x *= -1.0f;
	//处理球碰撞上下墙反弹逻辑
	if (ball[1].mBallPos.y <= thickness && ball[1].mBallVel.y < 0.0f)
		ball[1].mBallVel.y *= -1;
	else if (ball[1].mBallPos.y >= (768 - thickness) && ball[1].mBallVel.y > 0.0f)
		ball[1].mBallVel.y *= -1;
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
	//绘制球拍
	SDL_FRect paddle_P1{
		static_cast<int>(mPaddlePos_P1.x),
		static_cast<int>(mPaddlePos_P1.y - paddleH / 2),
		thickness,
		static_cast<int>(paddleH)
	};
	SDL_RenderFillRect(mRenderer, &paddle_P1);
	SDL_FRect paddle_P2{
		static_cast<int>(mPaddlePos_P2.x),
		static_cast<int>(mPaddlePos_P2.y - paddleH / 2),
		thickness,
		static_cast<int>(paddleH)
	};
	SDL_RenderFillRect(mRenderer, &paddle_P2);
	//绘制球
	SDL_FRect ball_1{
		static_cast<int>(ball[0].mBallPos.x - thickness / 2),
		static_cast<int>(ball[0].mBallPos.y - thickness / 2),
		thickness,
		thickness
	};
	SDL_RenderFillRect(mRenderer, &ball_1);
	SDL_FRect ball_2{
		static_cast<int>(ball[1].mBallPos.x - thickness / 2),
		static_cast<int>(ball[1].mBallPos.y - thickness / 2),
		thickness,
		thickness
	};
	SDL_RenderFillRect(mRenderer, &ball_2);
	//绘制图像
	SDL_RenderPresent(mRenderer);
}

void Game::Shutdown()
{	
	SDL_DestroyRenderer(mRenderer);	//删除渲染器
	SDL_DestroyWindow(mWindow);		//删除窗口
	SDL_Quit();						//推出游戏
}
