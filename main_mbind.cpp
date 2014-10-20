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
	using mymd::_X_;	using mymd::_x_;
	using m1 = mymd::mbind<is_ascendant, char, _X_<std::remove_reference>, int   , _X_<std::remove_reference>, _x_>;
	using m2 = m1::rebind<int , _X_<std::remove_reference>, double, _X_<std::remove_reference>, _x_>;

	using m3 = decltype(m1{} || m2{});
	std::cout << m3::apply<int&, sizeN<9>&, sizeN<16354>>::value << std::endl;
	std::cout << m3::apply<short&, sizeN<9>&, sizeN<16354>>::value << std::endl;

    using m4 = decltype(m1{} && m2{});
	std::cout << m4::apply<int&, sizeN<9>&, sizeN<16354>>::value << std::endl;
	std::cout << m4::apply<short&, sizeN<9>&, sizeN<16354>>::value << std::endl;
}

