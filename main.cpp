#include "rbind.h"
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
	using namespace std::placeholders;
	using namespace my::placeholders;
	Func f;
	int a = 2014, b = 2, c = 3, result = 0;
	//----------------------------------------------------------------------------------
	//basic   基本
	auto b1 = my::rbind(f, _1, _2, _3);
	result = b1(a, b, c);
	std::cout << "---- basic use" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	std::cout << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 2, c = 10;
	//preset default value     デフォルト値の設定
	auto b2 = my::rbind(f, a, _1, _2nd = 11);
	result = b2(b, c);
	std::cout << "---- do not use default value" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	a = 2014, b = 2, c = 10;
	result = b2(b);
	std::cout << "---- use default value for c(=11)" <<std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	std::cout << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 4, c = 30;
	//optional argument     引数省略
	auto b3 = my::rbind(_1, 2014, _2, !_3rd);
	result = b3(Func(), b);
	std::cout << "---- use default value of original functor for c(=1)" << std::endl;
	std::cout << "b = " << b << ", c = " << c << ", result = " << result << std::endl;
	std::cout << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 2, c = 1;
	std::string str("abcdefg");
	//convert argument by yield method     yieldメソッドによる引数変換
	auto b4 = my::rbind(f, 2014, _1,
								_2nd.yield<int>([](const std::string& s){ return s.length(); })
						);
	result = b4(b, str);
	std::cout << "---- convert augument by yield for the last argument" << std::endl;
	std::cout << "b = " << b << ", s = " << str << ", result = " << result << std::endl;
	//----------------------------------------------------------------------------------
	return 0;
}
