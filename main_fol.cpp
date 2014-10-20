//functor_overload header
//Copyright (c) 2014 mmYYmmdd

#include "functor_overload.hpp"

template <typename T>
using is_Nnumeric = mymd::is_some_rr<std::is_integral, std::is_floating_point>::apply<T>;

//c++14 の std::plus<void>の代替
    struct myPLus   {
        template<class T, class U>
            constexpr auto operator ()(const T& a, const U& b) const ->decltype(a+b)
        { return a + b; }
	};

#include <iostream>
#include <string>

int main()
{
    using namespace mymd;
    auto mm1 = gen<int&&, int&&>(std::plus<int>{})
               + gen<double&&, double&&>(std::minus<double>{})
               + gen<const std::string&, const std::string&>(std::plus<std::string>{});
    std::cout << mm1(10, 20) << " : (int a, int b) -> a + b" << std::endl;
    std::cout << mm1(5.55555, 6.666666) << " : (double a, double b) -> a - b" << std::endl;
    std::cout << mm1(std::string("abc_"), std::string("_xyz")) << " : (string a, string b) -> ab結合" << std::endl << std::endl;
    //-------------------------------------------------------------------
    auto mm2 =  gen<cond<is_Nnumeric>, cond<is_Nnumeric>>(myPLus{})
		        + genv<cond< >>(99999)
                + genn<0, cond<is_Nnumeric>, cond<is_Nnumeric, false>>()
                + genn<1, cond<is_Nnumeric, false>, cond<is_Nnumeric>>()
                + genv<cond<is_Nnumeric, false>, cond<is_Nnumeric, false>>(-1);
    std::cout << mm2(100, 200) << " : (numericなa, numericなb) -> a + b" << std::endl;
    std::cout << mm2(&mm1) << " : 引数が1つのとき無条件 -> 99999" << std::endl;
    std::cout << mm2(10.58, 8) << " : (numericなa, numericなb) -> a + b" << std::endl;
    std::cout << mm2(4, "XVY") << " : (numericなa, numericではないb) -> a"  << std::endl;
    std::cout << mm2(mm2, 8.5588) << " : (numericではないa, numericなb) -> b"  << std::endl;
    std::cout << mm2(std::string("123_"), std::string("_789")) << " : (numericではないa, numericではないb) -> -1"  << std::endl << std::endl;
}
