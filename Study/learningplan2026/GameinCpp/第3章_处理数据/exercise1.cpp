#include "exercises.h"
#include <iostream>

const float to_inch = 12;

void exercise1()
{
	float h;
	using namespace std;
	cout << "请输入你的身高：" << endl;
	cin >> h;
	cout << "你的英尺为" << h * to_inch << endl;
}