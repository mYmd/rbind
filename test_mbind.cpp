//sample for mbind header
//Copyright (c) 2014 mmYYmmdd

#include "mbind.hpp"
#include <iostream>

template <std::size_t N> struct sizeN { };
template <typename T>     struct size_of          { static const std::size_t value = sizeof(T); };
template <std::size_t N> struct size_of<sizeN<N>> { static const std::size_t value = N; };
//-----------------------
template <typename...>
struct is_ascendant	{
	static const bool value = true;
};
template <typename first, typename second, typename...tail>
struct is_ascendant<first, second, tail...>	{
	static const bool value = (size_of<first>::value <= size_of<second>::value) && is_ascendant<second, tail...>::value;
};
template <typename first, typename second>
struct is_ascendant<first, second>	{
	static const bool value = size_of<first>::value <= size_of<second>::value;
};
//********************************************************
int main()
{
	using mymd::_x_;	using mymd::_xrr_;
    {
        using m1 = mymd::mbind<is_ascendant, char, _xrr_, int   , _xrr_, _x_>;
        using m2 = m1::rebind<int , _xrr_, double, _xrr_, _x_>;

        using m3 = decltype(m1{} || m2{});
        std::cout << m3::apply<int&, sizeN<9>&, sizeN<16354>>::value << std::endl;
        std::cout << m3::apply<short&, sizeN<9>&, sizeN<16354>>::value << std::endl;

        using m4 = decltype(m1{} && m2{});
        std::cout << m4::apply<int&, sizeN<9>&, sizeN<16354>>::value << std::endl;
        std::cout << m4::apply<short&, sizeN<9>&, sizeN<16354>>::value << std::endl;
    }
    {
		auto m1 = mymd::mbind<std::is_integral, mymd::_xrr_>{} || mymd::mbind<std::is_floating_point, mymd::_xrr_>{};
		auto m2 = !mymd::mbind<std::is_same, float, mymd::_xrr_>{};
		auto m3 = mymd::mbind<std::is_same, std::string, mymd::_xrcvr_>{};
		using b = decltype(m1 && m2 || m3);
		std::cout << b::apply<int>::value << std::endl;
		std::cout << b::apply<int*>::value << std::endl;
		std::cout << b::apply<short>::value << std::endl;
		std::cout << b::apply<unsigned char>::value << std::endl;
		std::cout << b::apply<double>::value << std::endl;
		std::cout << b::apply<float>::value << std::endl;
		std::cout << b::apply<long long&>::value << std::endl;
		std::cout << b::apply<const std::string&>::value << std::endl;
	}

}
