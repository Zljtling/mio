#include "exercises.h"
#include <iostream>

void exercise5()
{
	float celsius, fahrenheit;
	using namespace std;
	cout << "Please enter a Celsius value: ";
	cin >> celsius;
	fahrenheit = 1.8 * celsius + 32.0;
	cout << celsius << " degrees Celsius is " << fahrenheit << " degrees Fahrenheit." << endl;
	return;
}