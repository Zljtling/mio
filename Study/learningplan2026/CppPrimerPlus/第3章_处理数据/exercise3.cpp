#include "exercises.h"
#include <iostream>

const float d_to_f = 60;
const float f_to_m = 60;

void exercise3()
{
	using namespace std;
	float degree, minute, second;
	cout << "Enter a latitude in degrees, minutes, and seconds:" << endl;
	cout << "First, enter the degrees:";
	cin >> degree;
	cout << "Next, enter the minutes of arc:";
	cin >> minute;
	cout << "Finally, enter the seconds of arc:";
	cin >> second;
	cout << degree << "degrees, " << minute << "minutes, " 
		<< second << " seconds = " << degree + minute / 60 + second / 3600 << "degrees" << endl;
}