#include "exercises.h"
#include <iostream>

const float km_to_yl = 0.6214;
const float jl_to_l = 3.785;

void exercise6()
{
	float yl, jl;
	using namespace std;
	cout << "请输入驱车里程: ";
	cin >> yl;
	cout << "请输入驱车汽油量: ";
	cin >> jl;
	cout << "耗油量一加仑的里程是: " << yl / jl / km_to_yl << endl;
	cout << "每100公里的耗油量是: " << jl * jl_to_l << endl;
}