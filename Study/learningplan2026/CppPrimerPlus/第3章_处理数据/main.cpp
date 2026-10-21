//#include "exercises.h"
//#include <iostream>
//#include <windows.h>
//
//int main() {
//    // 与编译选项 /utf-8 保持一致，正确显示中文。
//    SetConsoleOutputCP(CP_UTF8);
//
//    int choice;
//
//    do {
//        std::cout << "\n===== 练习题菜单 =====\n";
//        std::cout << "1. 练习题 1\n";
//        std::cout << "2. 练习题 2\n";
//        std::cout << "3. 练习题 3\n";
//        std::cout << "4. 练习题 4\n";
//        std::cout << "5. 练习题 5\n";
//        std::cout << "6. 练习题 6\n";
//        std::cout << "7. 练习题 7\n";
//        std::cout << "0. 退出程序\n";
//        std::cout << "请选择：";
//        std::cin >> choice;
//
//        switch (choice) {
//        case 1:
//            exercise1();
//            break;
//        case 2:
//            exercise2();
//            break;
//        case 3:
//            exercise3();
//            break;
//        case 4:
//            exercise4();
//            break;
//        case 5:
//            exercise5();
//            break;
//        case 6:
//            exercise6();
//            break;
//        case 7:
//            exercise7();
//            break;
//        case 0:
//            std::cout << "程序退出。\n";
//            break;
//        default:
//            std::cout << "无效选择，请重新输入。\n";
//            break;
//        }
//    } while (choice != 0);
//
//    return 0;
//}
#include <iostream>
#include <random>
int main() { std::mt19937 life{ std::random_device{}() }; std::uniform_int_distribution<int> fate(-10'000'000, 10'000'000); const int you = 0; int me = 0; for (int again = 0; again < 1'000'000; ++again) { me = fate(life); while (me != you)me = you + (me - you) / 2; }std::cout << (me == you ? "loveyou" : ""); }