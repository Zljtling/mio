#include "exercises.h"
#include <iostream>

const float km_to_yl = 0.6214;
const float jl_to_l = 3.785;

void exercise7()
{
	float km, jl;
	using namespace std;
	cout << "请输入公里: ";
	cin >> km;
	cout << "请输入加仑: ";
	cin >> jl;
	cout << "美国风格: " << km * km_to_yl << "/" << jl * jl_to_l << endl;
}