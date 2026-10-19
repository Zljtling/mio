#include "exercises.h"
#include <iostream>

const int second_to_minute = 60;
const int minute_to_hour = 60;
const int hour_to_day = 24;

void exercise4()
{
	using namespace std;
	long long seconds;
	cout << "Enter the number of seconds: ";
	cin >> seconds;
	cout << seconds << " seconds = " 
		<< seconds / second_to_minute / minute_to_hour / hour_to_day << " days, " 
		<< seconds / second_to_minute / minute_to_hour % hour_to_day << " hours, " 
		<< seconds / second_to_minute % minute_to_hour << " minutes, " 
		<< seconds %second_to_minute << " seconds" << endl;
}