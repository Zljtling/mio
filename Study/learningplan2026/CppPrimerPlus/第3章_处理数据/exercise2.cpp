#include "exercises.h"
#include <iostream>
#include <cmath>

void exercise2()
{
	float hight, weight, BMI;
	using namespace std;
	cout << "输入你的身高: ";
	cin >> hight;
	cout << "输入你的体重: ";
	cin >> weight;
	cout << "你的身高是：" << hight * 12 * 0.0254 << "米" << endl;
	cout << "你的体重是：" << weight / 2.2 << "千克" << endl;
	cout << "你的MBI是：" << (weight / 2.2) / sqrt(hight * 12 * 0.0254) << endl;	
}