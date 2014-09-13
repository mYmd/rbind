#include "rbindv.hpp"
#include <iostream>
#include <string>
#include <type_traits>

struct Func	{
	int operator ()(int a, int& b, int c = 1) const
		{ return a * 10000 + (b *= 100) + c; }
	int memFun(int a, int b, int c = 1) const
		{ return a * 10000 + b * 100 + c; }
};

template <template <typename> class Tr>
struct noref {
    template <typename V>
        struct apply    {
        static constexpr bool value = Tr<typename std::remove_reference<V>::type>::value;
    };
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
	auto b1_1 = mymd::rbind(f, until(_3rd));
	result = b1_1(a, b, c);
	std::cout << "---- basic use(placeholders range)" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 2, c = 10;
	//preset default value     デフォルト値の設定（実行時の値）
	auto b2 = mymd::rbind(f, a, _1, _2nd = b+9);
	result = b2(b, c);
	std::cout << "---- do not use default value" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	a = 2014, b = 2, c = 10;
	result = b2(b);
	std::cout << "---- use default value(runtime) for c(=11)" <<std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	//----------------------------------------------------------------------------------
	a = 2014, b = 4, c = 30;
	//optional argument     引数省略
	auto b3 = mymd::rbind(_1st, 2014, _2nd, _3rd);
	result = b3(Func(), b);
	std::cout << "---- use default value of original functor for c(=1)" << std::endl;
	std::cout << "b = " << b << ", c = " << c << ", result = " << result << std::endl;
	//----------------------------------------------------------------------------------
	//make nested dependent functor by operator *
    char s5[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    auto b5 = mymd::rbind(std::plus<std::string>{},
                          *mymd::emplace<std::string>(s5, _1),
                          *mymd::rbind([](auto b, auto e){ return std::string(b, e); }, _2, _3)
                          );
	std::cout << "---- nested dependent rbindt functor by operator*" << std::endl;
	std::cout << b5(4, s5+20, 5) << std::endl;
    //----------------------------------------------------------------------------------
	//parameter type assert   引数の型の制約
	auto b6 = mymd::rbind(f, _1, _2, _3rd.assert<noref<std::is_integral>>());
	result = b6(a, b=8, c=23);
	std::cout << "---- parameter type assert" << std::endl;
	std::cout << "a = " << a << ", b = " << b << ", c = " << c << ", result = " << result << std::endl;
	//----------------------------------------------------------------------------------
    return 0;
}
