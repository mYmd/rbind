//#pragma warning(disable : 4425)
#include "rbindv.hpp"	//#include "rbind.hpp"
#include <iostream>
#include <string>

struct Func	{
	int operator ()(int a, int& b, int c = 1) const
		{ return a * 10000 + (b *= 100) + c; }
	int memFun(int a, int b, int c = 1) const
		{ return a * 10000 + b * 100 + c; }
};

int main()
{
	using namespace std::placeholders;
	using namespace mymd::placeholders;
	Func f;
	int a = 2014, b = 2, c = 3, result = 0;
	//----------------------------------------------------------------------------------
	//basic   基本
	auto b1 = mymd::rbind(f, _1, _2, _3);
	result = b1(a, b, c);
	std::cout << "---- basic use" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	std::cout << std::endl;
	auto b1_1 = mymd::rbind(f, until(_3rd));
	result = b1_1(a, b, c);
	std::cout << "---- basic use(placeholders range)" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	std::cout << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 2, c = 10;
	//preset default value     デフォルト値の設定
	auto b2 = mymd::rbind(f, a, _1, _2nd = 11);
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
	auto b3 = mymd::rbind(_1st, 2014, _2nd, _3rd);
	result = b3(Func(), b);
	std::cout << "---- use default value of original functor for c(=1)" << std::endl;
	std::cout << "b = " << b << ", c = " << c << ", result = " << result << std::endl;
	std::cout << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 2, c = 1;
	std::string str("abcdefg");
	//convert argument by yield method     yieldメソッドによる引数変換
	auto b4 = mymd::rbind(&Func::memFun	,
			    &f			,
			    2014		,
			    _1			,
			    _2nd.yield([](const std::string& s){ return s.length(); })
			    );
	result = b4(b, str);
	std::cout << "---- convert augument by yield for the last argument" << std::endl;
	std::cout << "b = " << b << ", s = " << str << ", result = " << result << std::endl;
	//----------------------------------------------------------------------------------
	return 0;
}
