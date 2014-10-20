//functor_overload header
//Copyright (c) 2014 mmYYmmdd

#include "functor_overload.hpp"

template <typename T>
    struct is_Numeric   {
        static const bool value = std::is_integral<typename std::remove_reference<T>::type>::value ||
                                std::is_floating_point<typename std::remove_reference<T>::type>::value;
};

template <typename T>
    struct is_Pointer   {
        static const bool value = std::is_pointer<typename std::remove_reference<T>::type>::value;
};

struct Plus {
    template <typename T>
        auto operator()(T a, T b) const
        { return a + b; }
    template <typename T>
        auto operator()(T* a, T* b) const
        { return *a + *b; }
};

struct Minus {
    template <typename T>
        auto operator()(T a, T b) const
        { return a - b; }
    template <typename T>
        auto operator()(T* a, T* b) const
        { return *a - *b; }
};

#include <iostream>

int main()
{
    using namespace mymd;
    std::cout << "//数値／ポインタに対する計算をオーバーロードする人工的な例" << std::endl;
    std::cout << "//ふたつのファンクタのシグネチャは同一。それを意識的に選択する。" << std::endl;
    auto mm = gen<cond<is_Numeric>, cond<is_Numeric>>(Plus{} ) + 
              gen<cond<is_Pointer>, cond<is_Pointer>>(Minus{}) ;
    int i = 4, j = 5;
    double a = 3.676, b = 8.3212;
    std::cout << mm(i, j) << std::endl;
    std::cout << mm(a, b+0.0) << std::endl;
    std::cout << mm(&i, &j) << std::endl;
    std::cout << mm(&a, &b) << std::endl;
}    
