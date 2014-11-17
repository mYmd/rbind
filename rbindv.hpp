//rbindv header
//Copyright (c) 2014 mmYYmmdd

#if !defined MMYYMMDD_RBIND_AND_PLACEHOLDERS_INCLUDED
#define MMYYMMDD_RBIND_AND_PLACEHOLDERS_INCLUDED

#include <functional>

namespace mymd  {
namespace detail_bind   {

    struct nil  {};

    //============================================================================
    template <std::size_t... indices>
    struct indEx_sequence   { static constexpr std::size_t size() { return sizeof...(indices); } };

    // linear recursion is sufficient for bind parameters   線形再帰で十分
    template <std::size_t first, std::size_t last, typename result = indEx_sequence<>, bool flag = (first >= last)>
    struct indEx_range_imple    {   using type = result;    };

    template <std::size_t first, std::size_t last, std::size_t... indices>
    struct indEx_range_imple<first, last, indEx_sequence<indices...>, false> :
        indEx_range_imple<first + 1, last, indEx_sequence<indices..., first>>   {};

    template <std::size_t first, std::size_t last>
    using indEx_range = typename indEx_range_imple<first, last>::type;
    //---------------------------------------------------------------------
    template <bool...b> struct bool_arr{};
    template <bool, typename A>  struct type_pair {};
    template <typename A> A upcast(type_pair<true, A>&);
    //a little complicated because of VC++
    template <template <typename> class Pr>
    class find_unique     {
        template <typename... elem>
        struct apply0   {
            using b_array = bool_arr<Pr<elem>::value...>;
            template <typename, typename...>    struct D;
            template <bool...b, typename...T>
                struct D<bool_arr<b...>, T...> : type_pair<b, T>... {};
            static D<b_array, elem...>& getD();
                using type = decltype(upcast( getD() ));
        };
        template <typename Arr> struct Apply0;
        template <template <typename...> class Arr, typename... elem>
        struct Apply0<Arr<elem...>>  { using type = typename apply0<elem...>::type; };
    public:
        template <typename... elem> using apply = typename apply0<elem...>::type;
        template <typename Arr>     using Apply = typename Apply0<Arr>::type;
    };
    //+*******************************************************************************************
    // remove_rvalue_reference
    template<class T>
    struct remOve_rvaluE_reference  {   using type = T; };

    template<class T>
    struct remOve_rvaluE_reference<T&&> {   using type = T; };

    template<class T>
    using remOve_rvaluE_reference_t = typename remOve_rvaluE_reference<T>::type;

    //==================================================================
    //parameter buffer    パラメータバッファ
    template <typename T>
    struct param_buf    {
        T   val;        //std::unique_ptr<T>
        using type = T;
        param_buf(T&& t) : val(std::forward<T>(t))  { }
        T& get()    { return val; }
    };

    //************************************************************************************************
    //parameter adaptation judge by Tr<V>::value
    template <template <typename>class Tr>    struct condition_trait_1    {};
    //parameter adaptation judge by Tr::template apply<V>::value
    template <typename Tr>    struct condition_trait_2  {};
    //by copy
    struct by_copy    {};
    //************************************************************************************************

    //placeholder =======================================================================
    //with default parameter    デフォルト引数を付与されたplaceholder
    template <int N, typename T>
    struct placeholder_with_V   {
        mutable param_buf<T> elem;
        placeholder_with_V(T&& t) : elem(std::forward<T>(t))    {}
        T& get() const  { return elem.get(); }
    };

    //parameter type adaptation 1
    template <int N, template <typename>class Tr>
    struct placeholder_with_V<N, condition_trait_1<Tr>>     {};

    //parameter type adaptation 2
    template <int N, typename Tr>
    struct placeholder_with_V<N, condition_trait_2<Tr>>     {};

    //by copy
    template <int N>
    struct placeholder_with_V<N, by_copy>     {};

    //basic placeholder    基本のplaceholder
    template <int N>
    struct placeholder_with_V<N, void>  {
        constexpr placeholder_with_V()  {}
        // パラメータをコピー渡しにする     parameter by copy
        auto operator =(const placeholder_with_V<N, void>&) const->placeholder_with_V<N, by_copy>
            { return placeholder_with_V<N, by_copy>(); }
        //assign the default parameter (captured by reference)   デフォルト値を = で設定する（参照キャプチャ）
        template <typename V>
        auto operator =(V&& v) const -> placeholder_with_V<N, V>
            { return placeholder_with_V<N, V>(std::forward<V>(v)); }
        //assert parameter type  1
        template <template <typename>class Tr>
        auto assert() const ->placeholder_with_V<N, condition_trait_1<Tr>>
            { return placeholder_with_V<N, condition_trait_1<Tr>>{}; }
        //assert parameter type  2
        template <typename Tr>
        auto assert() const ->placeholder_with_V<N, condition_trait_2<Tr>>
            { return placeholder_with_V<N, condition_trait_2<Tr>>{}; }
        template <typename Tr>
        auto assert(Tr&& ) const ->placeholder_with_V<N, condition_trait_2<Tr>>
            { return placeholder_with_V<N, condition_trait_2<Tr>>{}; }
    };

    template <int N>
    using simple_placeholder = placeholder_with_V<N, void>;

    //=======================================================================================
    //convet from placeholder to argument    placeholderから実引数に変換 ====================
    //not my placeholder    mymd::detail_bind::placeholder以外
    template <typename T>
    struct parameter_evaluate   {
        template <typename V>
        struct eval     {
            using type = V;
            static V&& get(T& , V&& v)  { return std::forward<V>(v); }
        };
    };

    //with default value    デフォルト値を与えられた
    template <int N, typename T>
    struct parameter_evaluate<placeholder_with_V<N, T>>     {
        template <typename V, typename W = typename std::decay<V>::type>
        struct eval {           //if argument assigned    実引数あり
            using type = V;
            static V&& get(placeholder_with_V<N, T> const& , V&& v)
            { return std::forward<V>(v); }
        };
        template <typename V>
        struct eval<V, nil> {   //without argument    実引数なし → デフォルト値
            using type = T&;
            static T& get(placeholder_with_V<N, T> const& t, V&& )
            { return t.get(); }
        };
    };

    //assert parameter type 2
    template <int N, template <typename>class Tr>
    struct parameter_evaluate<placeholder_with_V<N, condition_trait_1<Tr>>> {
        template <typename V>
        struct eval     {
            using type = V;
            static V&& get(placeholder_with_V<N, condition_trait_1<Tr>> const& , V&& v)
            {
                static_assert(Tr<remOve_rvaluE_reference_t<V>>::value, "rbind:parameter type is rejected");
                return std::forward<V>(v);
            }
        };
    };

    //assert parameter type 2
    template <int N, typename Tr>
    struct parameter_evaluate<placeholder_with_V<N, condition_trait_2<Tr>>> {
        template <typename V>
        struct eval {
            using type = V;
            static V&& get(placeholder_with_V<N, condition_trait_2<Tr>> const& , V&& v)
            {
                static_assert(Tr::template apply<remOve_rvaluE_reference_t<V>>::value, "rbind:parameter type is rejected");
                return std::forward<V>(v);
            }
        };
    };

    //basic    基本
    template <int N>
    struct parameter_evaluate<simple_placeholder<N>>	{
        template <typename V>
        struct eval {
            using type = V;
            static V&& get(simple_placeholder<N> const& , V&& v)
            { return std::forward<V>(v); }
        };
    };

    //copy
    template <int N>
    struct parameter_evaluate<placeholder_with_V<N, by_copy>>	{
        template <typename V>
        struct eval {
            using type = typename std::decay<V>::type;
            static type get(placeholder_with_V<N, by_copy> const& , type v)
            { return v; }
        };
    };
    


    //************************************************************************************************
    //type of invoke    呼び出しの型
    //obj.*, pobj->*
    struct mem_c    {};
    //obj.*(), pobj->*()
    struct memF_c   {};
    //functor    ファンクタ型（フリー関数、ラムダ）
    struct fnc_c    {};
    //--------------------------------------------------------------------
    //tuple<type of invoke, return type, sizeof... parameters>    呼び出しの型
    template <typename C, typename R, std::size_t N>
    struct sig_type     {
        using call_type     =   C;
        using result_type   =   R;
        static const std::size_t value = N;
    };
    //---------------------------------------------------------------------------------
    //  [type0, type1, type2, ..., typeM, nil) ... typeN ,,,
    //       0,     1,     2, ....        M+1
    template<typename...>   struct nil_stop { static const std::size_t value = 0; };

    template<typename Head, typename... Tail>
    struct nil_stop<Head, Tail...>  {
        static const std::size_t value =
            std::is_same<typename std::decay<Head>::type, nil>::value?
                0 :  1 + nil_stop<Tail...>::value;
    };

    template<template <typename...> class Arr, typename... Params>
    struct nil_stop<Arr<Params...>>  {
        static const std::size_t value = nil_stop<Params...>::value;
    };

    //--------------------------------------------------------------------
    //handle a type as a raw ponter    スマートポインタ等を生ポインタと同様に扱うための変換 =======
    // int => nil*,  int* => int*,  smart_ptr<int> => int*,  itr<int> => int*
    template <typename T>
    class raw_ptr_type  {
        template <typename U>
        static std::true_type test1(U&& u, decltype(*u, static_cast<void>(0), 0) = 0);
        static std::false_type	test1(...);
        static const bool flag = decltype(test1(std::declval<T>()))::value;
        using psp_type = typename std::conditional<flag , T, nil*>::type;
    public:
        using type = decltype(&*std::declval<psp_type>());
    };

    //--------------------------------------------------------------------
    //determin the type of invocation
    template<typename TPL>
    class invokeType    {       //  SFINAE
        template<std::size_t N>
            static auto get()->decltype(std::declval<typename std::tuple_element<N, TPL>::type>());
        static const std::size_t ParamSize = nil_stop<TPL>::value;
        //
        template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
        static auto test(   indEx_sequence<indices1...> ,
                            indEx_sequence<indices2...> ,
                            T0&&                    t0  ,
                            T1&&                    t1  ,
                            T1P&&                   t1p ,
                            decltype(std::forward<T0>(t0)(get<indices1>()...), static_cast<void>(0), 1) = 0 )
            ->sig_type< fnc_c   ,
                        decltype(std::forward<T0>(t0)(get<indices1>()...))  ,
                        ParamSize - 1   >;
        //
        template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
        static auto test(   indEx_sequence<indices1...> ,
                            indEx_sequence<indices2...> ,
                            T0                      t0  ,
                            T1&&                    t1  ,
                            T1P&&                   t1p ,
                            decltype((std::forward<T1>(t1).*t0)(get<indices2>()...), static_cast<void>(0), 1) = 0   )
            ->sig_type<	memF_c  ,
                        decltype((std::forward<T1>(t1).*t0)(get<indices2>()...))    ,
                        ParamSize - 2   >;
        //
        template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
        static auto test(   indEx_sequence<indices1...> ,
                            indEx_sequence<indices2...> ,
                            T0                      t0  ,
                            T1&&                    t1  ,
                            T1P&&                   t1p,
                            decltype((std::forward<T1P>(t1p)->*t0)(get<indices2>()...), static_cast<void>(0), 1) = 0    )
            ->sig_type<	memF_c*     ,
                        decltype((std::forward<T1P>(t1p)->*t0)(get<indices2>()...)) ,
                        ParamSize - 2   >;
        //
        template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
        static auto test(   indEx_sequence<indices1...> ,
                            indEx_sequence<indices2...> ,
                            T0                      t0  ,
                            T1&&                    t1  ,
                            T1P&&                   t1p ,
                            decltype(std::forward<T1>(t1).*t0, static_cast<void>(0), 1) = 0 )
            ->sig_type<	mem_c   ,
                        decltype(std::forward<T1>(t1).*t0)  ,
                        ParamSize - 2   >;
        //
        template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
        static auto test(   indEx_sequence<indices1...> ,
                            indEx_sequence<indices2...> ,
                            T0                      t0,
                            T1&&                    t1  ,
                            T1P&&                   t1p ,
                            decltype(std::forward<T1P>(t1p)->*t0, static_cast<void>(0), 1) = 0  )
            ->sig_type<	mem_c*  ,
                        decltype(std::forward<T1P>(t1p)->*t0)   ,
                        ParamSize - 2   >;
        //
        using raw_p_t = typename raw_ptr_type<typename std::decay<typename std::tuple_element<1, TPL>::type>::type>::type;
        //
        using sig_t = decltype(test(indEx_range<1, ParamSize>{} ,
                                    indEx_range<2, ParamSize>{} ,
                                    get<0>()                    ,
                                    get<1>()                    ,
                                    std::declval<raw_p_t>()     ));
    public:
        using call_type     = typename sig_t::call_type;
        using result_type   = typename sig_t::result_type;
        using actual_indice = indEx_range<0, sig_t::value>;
    };

    //************************************************************************************************
    // execute
    template <typename R, typename T, typename A> struct executor;

    //member
    template <typename R, std::size_t... indices>
    struct executor<R, mem_c, indEx_sequence<indices...>>       {
        template <typename M, typename Obj>
        R exec(M mem, Obj&& obj) const
        { return (std::forward<Obj>(obj)).*mem; }
    };
    //----
    template <typename R, std::size_t... indices>
    struct executor<R, mem_c*, indEx_sequence<indices...>>      {
        template <typename M, typename pObj>
        R exec(M mem, pObj&& pobj) const
        { return (*std::forward<pObj>(pobj)).*mem; }
    };

    //member function
    template <typename R, std::size_t... indices>
    struct executor<R, memF_c, indEx_sequence<indices...>>  {
        template <typename M, typename Obj, typename... Params>
        R exec(M mem, Obj&& obj, Params&&... params) const
        {
            auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
            return ((std::forward<Obj>(obj)).*mem)(std::get<indices>(vt)...);
        }
    };
    //----
    template <typename R, std::size_t... indices>
    struct executor<R, memF_c*, indEx_sequence<indices...>> {
        template <typename M, typename pObj, typename... Params>
        R exec(M mem, pObj&& pobj, Params&&... params) const
        {
            auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
            return ((*std::forward<pObj>(pobj)).*mem)(std::get<indices>(vt)...);
        }
    };

    //functor
    template <typename R, std::size_t... indices>
    struct executor<R, fnc_c, indEx_sequence<indices...>>   {
        template <typename F, typename... Params>
        R exec(F&& f, Params&&... params) const
        {
            auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
            return (std::forward<F>(f))(std::get<indices>(vt)...);
        }
    };

    //************************************************************************************************
    //nested rbind to value at the same time  同時に評価するネストされたrbind
    template <typename... Vars> struct BindOfNested;

    template <typename> struct is_nested    { static const bool value = false; };

    template <typename... Vars>
	struct is_nested<BindOfNested<Vars...>> { static const bool value = true;  };

    //----------------------------------------------------------------------------
    //a parameter    パラメータ
    template <typename P>
    struct ParamOf  {
        mutable param_buf<P> elem;
        using p_h  = typename std::decay<P>::type;
        using param_t =  P;
        static const int placeholder = std::is_placeholder<p_h>::value;
        static const bool nested = is_nested<p_h>::value;
        ParamOf(P&& p) : elem(std::forward<P>(p))   {}
        P& get() const  { return elem.get(); }
    };
    //----------------------------------------------------------------------------
    template <typename P0, typename Params, std::size_t N, bool B, bool nest>
    class alt_param {
        using ph     = typename std::decay<typename P0::param_t>::type;
        using type_a = typename std::tuple_element<N-1, Params>::type;
        using vtype  = typename parameter_evaluate<ph>::template eval<type_a>;
    public:
        using type = typename vtype::type;
        static type get(const P0& p, Params&& params)
        { return vtype::get(p.get(), std::get<N-1>(std::forward<Params>(params))); }
    };
    //not placeholder
    template <typename P0, typename Params>
    struct alt_param<P0, Params, 0, true, false>    {
        using type = typename P0::param_t&;     //ここ
        static type get(const P0& p, Params&& )
        { return p.get(); }
    };
    //out of range
    template <typename P0, typename Params, std::size_t N>
    class alt_param<P0, Params, N, false, false>        {   //N is out of range
        using ph    = typename std::decay<typename P0::param_t>::type;
        using vtype = typename parameter_evaluate<ph>::template eval<nil>;
    public:
        using type = typename vtype::type;
        static type get(const P0& p, Params&& )
        { return vtype::get(p.get(), nil{}); }
    };
    //nested rbind to value at the same time  同時に評価するネストされたrbind
    template <typename P0, typename Params>
    class alt_param<P0, Params, 0, true, true>      {       //
        using nested = typename std::decay<typename P0::param_t>::type;
        static const std::size_t N = nested::N;
    public:
        using type = decltype(std::declval<nested>().invoke_imple(std::declval<Params>(), indEx_range<0, N>{}));
        static type get(const P0& p, Params&& params)
        { return p.get().invoke_imple(std::forward<Params>(params), indEx_range<0, N>{}); }
    };//*/
    //----------------------------------------------------------------------------
    template <std::size_t N, typename Params1, typename Params2>
    class param_result  {
        using P0 = typename std::tuple_element<N, Params1>::type;
        static const int	placeholder	= P0::placeholder;
        static const bool small = (placeholder <= std::tuple_size<Params2>::value);
        using alt_t = alt_param<P0, Params2, placeholder, small, P0::nested>;
    public:
        using result_type = typename alt_t::type;
        static result_type get(const Params1& params1, Params2&& params2)
        { return alt_t::get(std::get<N>(params1), std::forward<Params2>(params2)); }
    };

    template<std::size_t N, typename Params1, typename Params2>
    auto param_at(const Params1& params1, Params2&& params2)
        ->typename param_result<N, Params1, Params2>::result_type
    { return param_result<N, Params1, Params2>::get(params1, std::forward<Params2>(params2)); }

    //***************************************************************************************
    //main class    本体 ====================================================================
    template <typename... Vars>
    class   BindOf  {
        using Params1 = std::tuple<ParamOf<Vars>...>;
        Params1                                     params1;
        //-----------------------------------------------
        template<typename Params2T, typename D>     struct invoke_type_i;
        //------------
        template<typename Params2T, std::size_t... indices>
        struct invoke_type_i<Params2T, indEx_sequence<indices...>>  {
            static const Params1&	get1();
            using ParamTuple = decltype(std::forward_as_tuple(
                                    mymd::detail_bind::template param_at<indices>(get1(), std::declval<Params2T>())...)
                                    );
            using invoke_type   = typename mymd::detail_bind::invokeType<ParamTuple>;
            using actual_indice = typename invoke_type::actual_indice;
            using call_type     = typename invoke_type::call_type;
            using result_type   = typename invoke_type::result_type;
            using Executor_t    = executor<result_type, call_type, actual_indice>;
        };
        //
    protected:
        template<typename Params2T, std::size_t... indices>
        auto invoke_imple(Params2T&& params2, indEx_sequence<indices...> ) const
            ->typename invoke_type_i<Params2T, indEx_sequence<indices...>>::result_type
        {
            typename invoke_type_i<Params2T, indEx_sequence<indices...>>::Executor_t    Executor;
            return 
                Executor.exec(
                        param_at<indices>(params1, std::forward<Params2T>(params2))...
                );
        }
    public:
        template <std::size_t... indices>
        BindOf(std::tuple<Vars...>&& vars, indEx_sequence<indices...>) :
            params1(std::get<indices>(std::forward<std::tuple<Vars...>>(vars))...)  {}
        //
        static const std::size_t  N   =  sizeof...(Vars);
        //
        template <typename... Params2>
        auto operator ()(Params2&&... params2) const
            ->typename invoke_type_i<std::tuple<Params2...>, indEx_range<0, N>>::result_type
        {
            return invoke_imple(    std::forward_as_tuple(std::forward<Params2>(params2)...),
                                indEx_range<0, N>{}
                            );
        }
        //
        auto operator *() const ->BindOfNested<Vars...>;
    };

    //BindOf is not a pointer type       BindOfはポインタじゃない
    template <typename... Vars>
    class raw_ptr_type<BindOf<Vars...>>     {   public: using type = nil*;  };

    //*******************************************************************
    //nested rbind to value at the same time  同時に評価するネストされたrbind
    template <typename... Vars>
    struct BindOfNested : private BindOf<Vars...>   {
        BindOfNested(const BindOf<Vars...>& b) : BindOf<Vars...>(b)         {}
        BindOfNested(BindOf<Vars...>&& b) : BindOf<Vars...>(std::move(b))   {}
        using BindOf<Vars...>::invoke_imple;
        static const std::size_t    N =  sizeof...(Vars);
    };

    template <typename... Vars>
    auto BindOf<Vars...>::operator *() const ->BindOfNested<Vars...>
    { return BindOfNested<Vars...>(*this); }

    //*******************************************************************
    template <typename T>
    auto granulate(T&& t)->std::tuple<T&&>
    { return std::forward_as_tuple(std::forward<T>(t)); }

    template <int... indices>
    auto granulate(indEx_sequence<indices...>&& )
        -> std::tuple<placeholder_with_V<indices, void>...>
    { return std::tuple<placeholder_with_V<indices, void>...>(); };

    template <typename... T>
    auto untie_vars(T&&... t)->decltype(std::tuple_cat(granulate(std::forward<T>(t))...))
    { return std::tuple_cat(granulate(std::forward<T>(t))...); }
    //-------------------------------------------------
    template <typename... Vars>
    auto rbind_imple(std::tuple<Vars...>&& vars)->BindOf<remOve_rvaluE_reference_t<Vars>...>
    {
        using index = indEx_range<0, sizeof...(Vars)>;
        return BindOf<remOve_rvaluE_reference_t<Vars>...>(std::forward<std::tuple<Vars...>>(vars), index());
    }
    //-------------------------------------------------
    template <typename T>
    struct emplace_t    {
        template <typename... Vars>
        T operator()(Vars&&... vars) const
        { return T(std::forward<Vars>(vars)...); }
    };

} //namespace mymd::detail_bind

    //predefined placeholders _1st, _2nd, _3rd, _4th, ...      定義済みプレースホルダ 
    namespace placeholders  {
        namespace   {
            constexpr detail_bind::simple_placeholder< 1>  _1st;
            constexpr detail_bind::simple_placeholder< 2>  _2nd;
            constexpr detail_bind::simple_placeholder< 3>  _3rd;
            constexpr detail_bind::simple_placeholder< 4>  _4th;
            constexpr detail_bind::simple_placeholder< 5>  _5th;
            constexpr detail_bind::simple_placeholder< 6>  _6th;
            constexpr detail_bind::simple_placeholder< 7>  _7th;
            constexpr detail_bind::simple_placeholder< 8>  _8th;
            constexpr detail_bind::simple_placeholder< 9>  _9th;
            constexpr detail_bind::simple_placeholder<10> _10th;
            constexpr detail_bind::simple_placeholder<11> _11th;
            constexpr detail_bind::simple_placeholder<12> _12th;
            constexpr detail_bind::simple_placeholder<13> _13th;
            constexpr detail_bind::simple_placeholder<14> _14th;
            constexpr detail_bind::simple_placeholder<15> _15th;
            //constexpr detail_bind::simple_placeholder<INT_MAX> _oo;
        }
        // copy any parameter
        template <typename T> T&& _val(T&& x)      { return std::forward<T>(x); }
        template <typename T> T   _val(T& x)       { return x; }
        template <typename T> T   _val(const T& x) { return x; }
        // Alias for placeholder
        template <int N>
        using plhdr_t = detail_bind::simple_placeholder<N>;
    }   // mymd::placeholders

    namespace detail_bind   {
        //until(_9th) => [_1st, _2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
        //this will be converted to tuple of placeholders by granulate function afterward
        // default parameter T is a workaround for visual c++ 2013 CTP
        template <int N, typename T = void>
        auto until(const placeholder_with_V<N, T>& ) ->indEx_range<1, N+1>
        { return indEx_range<1, N+1>{}; }

        //range(_2nd, _9th) => [_2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
        //this will be converted to tuple of placeholders by granulate function afterward
        // default parameter T is a workaround for visual c++ 2013 CTP
        template <int M, int N, typename T = void>
        auto range(const placeholder_with_V<M, T>&, const placeholder_with_V<N, T>& ) ->indEx_range<M, N+1>
        { return indEx_range<M, N+1>{}; }
    }   // mymd::detail_bind

    //************************************************************************
    //user interface / rbind functions       ユーザが使う rbind関数
    template <typename... Vars>
    auto rbind(Vars&&... vars)
        ->decltype(detail_bind::rbind_imple(detail_bind::untie_vars(std::forward<Vars>(vars)...)))
    { return detail_bind::rbind_imple(detail_bind::untie_vars(std::forward<Vars>(vars)...)); }

    //construct of typeT   型Tのコンストラクタ呼び出し
    template <typename T, typename... Vars>
    auto emplace(Vars&&... vars) ->decltype(rbind(detail_bind::emplace_t<T>{}, std::forward<Vars>(vars)...))
    { return rbind(detail_bind::emplace_t<T>{}, std::forward<Vars>(vars)...); }

} //namespace mymd

//***********************************************************************************************

namespace std {
    template <int N, typename T>
    struct is_placeholder<mymd::detail_bind::placeholder_with_V<N, T>> : std::integral_constant<int, N> {};
    //#ifdef BOOST_BIND_ARG_HPP_INCLUDED
    //template <> struct is_placeholder<boost::arg<1>> : integral_constant<int, 1> { };
    //...
}   //namespace std

#endif
