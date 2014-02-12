//rbind header
//Copyright (c) 2014 Masahiko Yamada

#pragma once
#include <functional>

namespace my	{
namespace detail	{

struct nil	{
	typedef nil po_t;
	typedef nil back_t;
	typedef nil param_t;
};

//number of arguments    引数の数
template <int N> struct p_n	{
	static const int value = N;
};

//type of invoke    呼び出しの型
	//obj.*, pobj->*
struct mem_c	{	};
	//obj.*(), pobj->*()
struct memF_c	{	};
	//functor    ファンクタ型（フリー関数、ラムダ）
struct fnc_c	{	};

//pair <type of invoke, return type>    呼び出しの型と戻り値型のペア
template <typename C, typename R>
	struct cr_pair	{
		typedef C call_type;
		typedef R result_type;
	};

//std::remove_reference && std::remove_cv
template<typename T>
	struct remove_ref_cv	{
		typedef typename std::remove_reference<typename std::remove_cv<T>::type>::type	type;
	};
// type compair without const/volatile and reference   cv属性と参照属性を外して型の同一性を比較
template<typename T, typename U>
	struct is_same_ignoring_cv_ref :
		std::is_same<typename remove_ref_cv<T>::type, typename remove_ref_cv<U>::type>	{	};

//parameter buffer    パラメータバッファ
template <typename T>
	struct param_buf	{
		mutable T		val;
		typedef T		type;
		T& get() const	{ return val; }
		param_buf(T&& t) : val(std::forward<T>(t))				{ }
		param_buf(param_buf<T>&& x) : val(std::forward<T>(x.val))	{ }
		param_buf(const param_buf<T>& x) : val(x.val)					{ }
	};
template <typename T>
	struct param_buf<T&>	{
		T&				val;
		typedef T&		type;
		T& get() const				{ return val; }
		param_buf(T& t) : val(t)					{	}
		param_buf(const param_buf<T&>& x) : val(x.val) {	}
	};

//placeholder
	//yielded     _1st.yield<T>(functor)    値を評価するplaceholder
template <int N, typename R, typename F>
	struct placeholder_with_F : param_buf<F>	{
		placeholder_with_F(F&& f) : param_buf<F>(std::forward<F>(f))		{ }
		template <typename V>
			R eval(V&& v) const	{ return (param_buf<F>::get())(std::forward<V>(v)); }
	};
	//yielded    _2nd.yield<T>()    statc_castするplaceholder
template <int N, typename R>
	struct placeholder_with_F<N, R, void>		{	};
	//with default parameter    デフォルト引数を付与されたplaceholder
template <int N, typename T>
	struct placeholder_with_F<N, void, T> : param_buf<T>	{
		placeholder_with_F(T&& t) : param_buf<T>(std::forward<T>(t)) { }
	};
	//basic placeholder    基本のplaceholder
template <int N>
	struct placeholder_with_F<N, void, void>	{
		//assign the default parameter    デフォルト値を = で設定する
		template <typename V>
			auto operator =(V&& v) const -> placeholder_with_F<N, void, V>
			{ return placeholder_with_F<N, void, V>(std::forward<V>(v)); }
		//<workarouns for VC> ------------------------------------------------------------
		template <typename V>
			auto  operator =(const V&& v) const -> placeholder_with_F<N, void, const V&>
			{ return placeholder_with_F<N, void, const V&>(v); }
		//</workarouns for VC> -----------------------------------------------------------
		//yield
		template <typename R, typename F>
			auto yield(F&& f) const ->placeholder_with_F<N, R, F>
			{ return placeholder_with_F<N, R, F>(std::forward<F>(f)); }
		template <typename R>
			auto yield(void) const ->placeholder_with_F<N, R, void>
			{ return placeholder_with_F<N, R, void>(); }
		//optional    引数を必須としない（元のファンクタのデフォルト引数を使う等）
		auto operator !(void) const ->placeholder_with_F<N, void, nil*>
			{ return placeholder_with_F<N, void, nil*>((nil*)0); }
	};

//convet from placeholder to argument    placeholderから実引数に変換
	//no placeholder    my::detail::placeholder以外
template <typename T>
	struct parameter_evaluate	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(T& t, V&& v)
				{	return std::forward<V>(v);	}
			};
	};
	//yieldef with a functor    ファンクタからyieldされた
template <int N, typename F, typename R>
	struct parameter_evaluate<placeholder_with_F<N, R, F>>	{
		template <typename V>
			struct eval	{
				typedef R	type;
				static R get(placeholder_with_F<N, R, F>& t, V&& v)
				{	return t.eval(std::forward<V>(v));	}
			};
	};
	//static_cast
template <int N, typename R>
	struct parameter_evaluate<placeholder_with_F<N, R, void>>	{
		template <typename V>
			struct eval	{
				typedef R	type;
				static R get(placeholder_with_F<N, R, void>& t, V&& v)
				{	return static_cast<R>(std::forward<V>(v));	}
			};
	};
	//with default value    デフォルト値を与えられた
template <int N, typename T>
	struct parameter_evaluate<placeholder_with_F<N, void, T>>	{
		template <typename V, typename W = typename remove_ref_cv<V>::type>
			struct eval	{			//if argument assigned    実引数あり
				typedef V	type;
				static V&& get(placeholder_with_F<N, void, T>& t, V&& v)
				{	return std::forward<V>(v);	}
			};
		template <typename V>
			struct eval<V, nil>	{	//without argument    実引数なし → デフォルト値
				typedef T&	type;
				static T& get(placeholder_with_F<N, void, T>& t, V&& v)
				{	return t.get();	}
			};
	};
	//optional    引数を必須としない（元のファンクタのデフォルト引数を使う等）
template <int N>
	struct parameter_evaluate<placeholder_with_F<N, void, nil*>>	{
		template <typename V, typename W = typename remove_ref_cv<V>::type>
			struct eval	{			//if argument assigned    実引数あり
				typedef V	type;
				static V&& get(placeholder_with_F<N, void, nil*>& t, V&& v)
				{	return std::forward<V>(v);	}
			};
		template <typename V>
			struct eval<V, nil>	{	//without argument    実引数なし → 元のファンクタのデフォルト値
				typedef nil*	type;
				static nil* get(placeholder_with_F<N, void, nil*>& t, V&& v)
				{	return (nil*)0;	}
			};
	};
	//basic    基本
template <int N>
	struct parameter_evaluate<placeholder_with_F<N, void, void>>	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(placeholder_with_F<N, void, void>& t, V&& v)
				{	return std::forward<V>(v);	}
			};
	};

//deal it as a raw ponter    スマートポインタ等を生ポインタと同様に扱うための変換
//T:int => nil*,  int* => int*,  smart_ptr<int> => int*
template <typename T>
	class raw_ptr_type	{
		static T getT();
		template <typename U>
			static std::true_type test1(U&& u, decltype(*u, 0) = 0);
		static std::false_type	test1(...);
		//<workaround for VC2012> -----------------------------------------------------
		template <typename U>
			static std::true_type test2(U&& u,
				typename std::enable_if<is_same_ignoring_cv_ref<decltype(&*u), decltype(&u)>::value>::type* = 0);
		static std::false_type	test2(...);
		//</workaround for VC2012> ----------------------------------------------------
		typedef decltype(test1(getT()))	T1;
		typedef decltype(test2(getT()))	T2;
		typedef typename std::conditional<T1::value && !T2::value, T, nil*>::type  psp_type; 
		static psp_type getPSP();
	public:
		typedef decltype(&*getPSP())	type;		//T:int=>nil*, int*=>int*, s_ptr<int>=>int*
	};

template <int N, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5,
														typename A6, typename A7, typename A8, typename A9>
	struct n_th_type	{
		typedef typename std::conditional<N == 0, A0,
					typename std::conditional<N == 1, A1,
						typename std::conditional<N == 2, A2,
							typename std::conditional<N == 3, A3,
								typename std::conditional<N == 4, A4,
									typename std::conditional<N == 5, A5,
										typename std::conditional<N == 6, A6,
											typename std::conditional<N == 7, A7,
												typename std::conditional<N == 8, A8,
													typename std::conditional<N == 9, A9,
														nil
							>::type>::type>::type>::type>::type>::type>::type>::type>::type>::type
				type;
	};

//convert from placeholder to it    placeholderなら実引数もしくはデフォルト引数に変換
template <typename P, typename A1, typename A2, typename A3, typename A4, typename A5,
												typename A6, typename A7, typename A8, typename A9>
	class alt_type	{
		typedef typename remove_ref_cv<P>::type		ph;
		static const int N = std::is_placeholder<ph>::value;
		typedef typename n_th_type<N, P&, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type		type_a;	//P&
		typedef typename parameter_evaluate<ph>::template eval<type_a>::type	type0;
	public:
		typedef typename std::conditional<std::is_same<type0, nil*>::value, nil, type0>::type type;
	};

//no nil type before N    nilではないNより前の型
template <int N, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5,
												typename A6, typename A7, typename A8, typename A9>
	struct nil_bound	{
		typedef typename n_th_type<N, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type	type;
		static const int value = std::is_same<nil, type>::value?
									nil_bound<N-1, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9>::value:
									N;
	};
template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5,
												typename A6, typename A7, typename A8, typename A9>
	struct nil_bound<0, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9>	{
		static const int value = 0;
	};
//************************************************************************************************
//argument type and result type / functor or mem fun   引数型と戻値型とファンクタ／メンバ関数の区別
template <typename F, typename P1, typename P2, typename P3, typename P4, typename P5,
												typename P6, typename P7, typename P8, typename P9>
struct func_signature_base0	{
	typedef F	fn_t;
	typedef P1 p1_t; typedef P2 p2_t; typedef P3 p3_t; typedef P4 p4_t;
	typedef P5 p5_t; typedef P6 p6_t; typedef P7 p7_t; typedef P8 p8_t; typedef P9 p9_t;
	static F  getF();
	static P1 get1(); static P2 get2(); static P3 get3(); static P4 get4();
	static P5 get5(); static P6 get6(); static P7 get7(); static P8 get8(); static P9 get9();
	static typename raw_ptr_type<P1>::type	ptr1();
	//N of p_n<N> is number of auguments     p_n<N>のNは引数の数
	template <typename V> static	auto
		test(V&& v, p_n<9>, decltype(   v(get1(), get2(), get3(), get4(), get5(), get6(), get7(), get8(), get9()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3(), get4(), get5(), get6(), get7(), get8(), get9()))>;
	template <typename V> static	auto
		test(V&& v, p_n<9>, decltype( (get1().*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8(), get9()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8(), get9()))>;
	template <typename V> static	auto
		test(V&& v, p_n<9>, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8(), get9()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8(), get9()))>;
	template <typename V> static	auto
		test(V&& v, p_n<8>, decltype(   v(get1(), get2(), get3(), get4(), get5(), get6(), get7(), get8()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3(), get4(), get5(), get6(), get7(), get8()))>;
	template <typename V> static	auto
		test(V&& v, p_n<8>, decltype( (get1().*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8()))>;
	template <typename V> static	auto
		test(V&& v, p_n<8>, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6(), get7(), get8()))>;
	template <typename V> static	auto
		test(V&& v, p_n<7>, decltype(   v(get1(), get2(), get3(), get4(), get5(), get6(), get7()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3(), get4(), get5(), get6(), get7()))>;
	template <typename V> static	auto
		test(V&& v, p_n<7>, decltype( (get1().*v)(get2(), get3(), get4(), get5(), get6(), get7()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3(), get4(), get5(), get6(), get7()))>;
	template <typename V> static	auto
		test(V&& v, p_n<7>, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6(), get7()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6(), get7()))>;
	template <typename V> static	auto
		test(V&& v, p_n<6>, decltype(   v(get1(), get2(), get3(), get4(), get5(), get6()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3(), get4(), get5(), get6()))>;
	template <typename V> static	auto
		test(V&& v, p_n<6>, decltype( (get1().*v)(get2(), get3(), get4(), get5(), get6()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3(), get4(), get5(), get6()))>;
	template <typename V> static	auto
		test(V&& v, p_n<6>, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3(), get4(), get5(), get6()))>;
	template <typename V> static	auto
		test(V&& v, p_n<5>, decltype(   v(get1(), get2(), get3(), get4(), get5()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3(), get4(), get5()))>;
	template <typename V> static	auto
		test(V&& v, p_n<5>, decltype( (get1().*v)(get2(), get3(), get4(), get5()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3(), get4(), get5()))>;
	template <typename V> static	auto
		test(V&& v, p_n<5>, decltype((ptr1()->*v)(get2(), get3(), get4(), get5()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3(), get4(), get5()))>;
	template <typename V> static	auto
		test(V&& v, p_n<4>, decltype(   v(get1(), get2(), get3(), get4()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3(), get4()))>;
	template <typename V> static	auto
		test(V&& v, p_n<4>, decltype( (get1().*v)(get2(), get3(), get4()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3(), get4()))>;
	template <typename V> static	auto
		test(V&& v, p_n<4>, decltype((ptr1()->*v)(get2(), get3(), get4()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3(), get4()))>;
	template <typename V> static	auto
		test(V&& v, p_n<3>, decltype(   v(get1(), get2(), get3()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2(), get3()))>;
	template <typename V> static	auto
		test(V&& v, p_n<3>, decltype( (get1().*v)(get2(), get3()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2(), get3()))>;
	template <typename V> static	auto
		test(V&& v, p_n<3>, decltype((ptr1()->*v)(get2(), get3()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2(), get3()))>;
	template <typename V> static	auto
		test(V&& v, p_n<2>, decltype(   v(get1(), get2()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1(), get2()))>;
	template <typename V> static	auto
		test(V&& v, p_n<2>, decltype( (get1().*v)(get2()), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)(get2()))>;
	template <typename V> static	auto
		test(V&& v, p_n<2>, decltype((ptr1()->*v)(get2()), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)(get2()))>;
	template <typename V> static	auto
		test(V&& v, p_n<1>, decltype(     v(get1()), 1) = 0)
			-> cr_pair<fnc_c, decltype(v(get1()))>;
	template <typename V> static	auto
		test(V&& v, p_n<1>, decltype( (get1().*v)(), 1) = 0)
			-> cr_pair<memF_c, decltype((get1().*v)())>;
	template <typename V> static	auto
		test(V&& v, p_n<1>, decltype((ptr1()->*v)(), 1) = 0)
			-> cr_pair<memF_c*, decltype((ptr1()->*v)())>;
	template <typename V> static	auto
		test(V&& v, p_n<1>, decltype( get1().*v, 1) = 0)
			-> cr_pair<mem_c, decltype(get1().*v)>;
	template <typename V> static	auto
		test(V&& v, p_n<1>, decltype(ptr1()->*v, 1) = 0)
			-> cr_pair<mem_c*, decltype(ptr1()->*v)>;
	template <typename V> static	auto
		test(V&& v, p_n<0>, decltype(v(), 1) = 0)
			-> cr_pair<fnc_c, decltype(v())>;
	static auto test(...)
			-> cr_pair<nil, nil>;
	};
//augument type and result type    引数型と戻値型
template <int N , typename F, typename P1, typename P2, typename P3, typename P4, typename P5,
														typename P6, typename P7, typename P8, typename P9>
	struct func_signature_base : func_signature_base0<F, P1, P2, P3, P4, P5, P6, P7, P8, P9>	{
		typedef func_signature_base0<F, P1, P2, P3, P4, P5, P6, P7, P8, P9>		B;
		static const int NN = nil_bound<N, F, P1, P2, P3, P4, P5, P6, P7, P8, P9>::value; 
		typedef decltype(B::test(B::getF(), p_n<NN>()))						cr_type;
		typedef typename cr_type::call_type									call_t;
		typedef typename cr_type::result_type								result_t;
	};
//the set of augument types    引数集合を決定------------------------------------------------------
template <int N,
	typename F, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9,
	typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9 >
	struct func_signature : func_signature_base <N																,
											typename alt_type<F , A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
										typename alt_type<P1, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
									typename alt_type<P2, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
								typename alt_type<P3, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
							typename alt_type<P4, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
						typename alt_type<P5, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
					typename alt_type<P6, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
				typename alt_type<P7, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
			typename alt_type<P8, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type ,
		typename alt_type<P9, A1, A2, A3, A4, A5, A6, A7, A8, A9>::type >
	{	};
//************************************************************************************************
//arguments    パラメータ
template <typename P, int N>
	struct ParamOf : param_buf<P>	{
		typedef param_buf<P> base;
		typedef P param_t;
		typedef typename remove_ref_cv<P>::type		p_h;
		static const int placeholder = std::is_placeholder<p_h>::value;
		ParamOf(P&& p) : base(std::forward<P>(p))	{	}
		ParamOf(ParamOf<P, N>&& p) : base(std::forward<base>(p))	{	}
		ParamOf(const ParamOf<P, N>& p) : base(p)	{	}
	};
//------------------------------------------------------------------------------------------------
//convert from placeholder to argument     placeholderだったら実引数に変換する SFINAE
template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct passer	{
	nil* operator ()(const nil*, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9)
		{	return (nil*)(0);	}
	template <typename P>	//return ref    参照を返す
		typename P::param_t& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==0>::type* = 0)
		{	return p->get();	}
	template <typename P>
		A1&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==1>::type* = 0)
		{	return std::forward<A1>(a1);	}
	template <typename P>
		A2&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==2>::type* = 0)
		{	return std::forward<A2>(a2);	}
	template <typename P>
		A3&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==3>::type* = 0)
		{	return std::forward<A3>(a3);	}
	template <typename P>
		A4&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==4>::type* = 0)
		{	return std::forward<A4>(a4);	}
	template <typename P>
		A5&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==5>::type* = 0)
		{	return std::forward<A5>(a5);	}
	template <typename P>
		A6&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==6>::type* = 0)
		{	return std::forward<A6>(a6);	}
	template <typename P>
		A7&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==7>::type* = 0)
		{	return std::forward<A7>(a7);	}
	template <typename P>
		A8&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==8>::type* = 0)
		{	return std::forward<A8>(a8);	}
	template <typename P>
		A9&& operator ()(P* p, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9,
			typename std::enable_if<P::placeholder==9>::type* = 0)
		{	return std::forward<A9>(a9);	}
	//------------------------------------------------------------
	template <typename V>
		nil* operator ()(const nil*, V&&)
		{	return (nil*)(0);	}
	template <typename P, typename V>
		auto operator ()(P* p, V&& x)
			-> typename parameter_evaluate<typename P::param_t>::template eval<V>::type
		{	typedef typename parameter_evaluate<typename P::param_t>::template eval<V>	vtype;
			return vtype::get(p->get(), std::forward<V>(x));		}
};
//************************************************************************************************
template <typename S, typename T> struct executer;
//************************************************************************************************

//main class    本体
template <typename P, typename T = nil, int N = 0>
struct BindOf : protected ParamOf<P, N>, protected T	{
protected:
	typedef		T		back_t		;
	static	const int		depth = N	;
	typedef ParamOf<P, N>		po_t		;
	typedef	BindOf<P, T, N>			self_t		;
	typedef typename po_t::param_t		param_t		;
	//---------------------------------------------------
	template <int L, typename V = self_t, typename D = std::true_type>
		struct Lv
		{  typedef typename Lv<L+1, V>::type::back_t  type;  };
	template <int L, typename V>
		struct Lv<L, V, std::integral_constant<bool, L == V::depth>>
		{  typedef V  type;  };
	template <int L, typename V>
		struct Lv<L, V, std::integral_constant<bool, L >= V::depth+1>>
		{  typedef typename my::detail::nil type;  };
	//----------------------------------------------------------------------------------------------------------------
	template <typename A1=my::detail::nil, typename A2=my::detail::nil, typename A3=my::detail::nil,
				typename A4=my::detail::nil, typename A5=my::detail::nil, typename A6=my::detail::nil,
					typename A7=my::detail::nil, typename A8=my::detail::nil, typename A9=my::detail::nil>
		struct mySignature	{
			typedef typename my::detail::func_signature	<
								depth						,
									typename Lv<0>::type::param_t,
										typename Lv<1>::type::param_t,
											typename Lv<2>::type::param_t,
												typename Lv<3>::type::param_t,
													typename Lv<4>::type::param_t,
														typename Lv<5>::type::param_t,
															typename Lv<6>::type::param_t,
																typename Lv<7>::type::param_t,
																	typename Lv<8>::type::param_t,
																		typename Lv<9>::type::param_t,
									A1, A2, A3, A4, A5, A6, A7, A8, A9>
						type;
		};
public:
	BindOf(P&& p) : ParamOf<P, N>(std::forward<P>(p)), back_t()	   	{ }
	BindOf(const self_t& t) : ParamOf<P, N>(t), back_t(t)						{ }
	BindOf(P&& p, T&& t) : ParamOf<P, N>(std::forward<P>(p)), back_t(std::forward<T>(t))	{ }
	BindOf(self_t&& t) : ParamOf<P, N>(std::forward<ParamOf<P, N>>(t)), back_t(std::forward<T>(t))		{ }
	// operator(a,b,c,d,e,f,g,h,i)
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8, A9&& a9) const
			->typename mySignature<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type::result_t
		{
		typedef typename mySignature<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type		signature;
		typedef typename signature::call_t								ct;
		passer<A1, A2, A3, A4, A5, A6, A7, A8, A9>						pass;
		executer<signature, typename std::remove_pointer<ct>::type>		theExecuter;
		return theExecuter.exec
			(
			pass(static_cast<typename Lv<0>::type::po_t const*>(this),
				pass(static_cast<typename Lv<0>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<1>::type::po_t const*>(this),
				pass(static_cast<typename Lv<1>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<2>::type::po_t const*>(this),
				pass(static_cast<typename Lv<2>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<3>::type::po_t const*>(this),
				pass(static_cast<typename Lv<3>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<4>::type::po_t const*>(this),
				pass(static_cast<typename Lv<4>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<5>::type::po_t const*>(this),
				pass(static_cast<typename Lv<5>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<6>::type::po_t const*>(this),
				pass(static_cast<typename Lv<6>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<7>::type::po_t const*>(this),
				pass(static_cast<typename Lv<7>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<8>::type::po_t const*>(this),
				pass(static_cast<typename Lv<8>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) ),
			pass(static_cast<typename Lv<9>::type::po_t const*>(this),
				pass(static_cast<typename Lv<9>::type::po_t const*>(this),
						std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						std::forward<A7>(a7), std::forward<A8>(a8), std::forward<A9>(a9) ) )
			);
		}
	// operator(a,b,c,d,e,f,g,h)
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7, A8&& a8) const
			->typename mySignature<A1, A2, A3, A4, A5, A6, A7, A8>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						   std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						   std::forward<A7>(a7), std::forward<A8>(a8), nil()				);	}
	// operator(a,b,c,d,e,f,g)
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6, A7&& a7) const
			->typename mySignature<A1, A2, A3, A4, A5, A6, A7>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						   std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						   std::forward<A7>(a7), nil(),				   nil()				);	}
	// operator(a,b,c,d,e,f)
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, A6&& a6) const
			->typename mySignature<A1, A2, A3, A4, A5, A6>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						   std::forward<A4>(a4), std::forward<A5>(a5), std::forward<A6>(a6),
						   nil()			   , nil()				 , nil()				);	}
	// operator(a,b,c,d,e)
	template <typename A1, typename A2, typename A3, typename A4, typename A5>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5) const
			->typename mySignature<A1, A2, A3, A4, A5>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						   std::forward<A4>(a4), std::forward<A5>(a5), nil()			   ,
						   nil()			   , nil()				 , nil()				);	}
	// operator(a,b,c,d)
	template <typename A1, typename A2, typename A3, typename A4>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3, A4&& a4) const
			->typename mySignature<A1, A2, A3, A4>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						   std::forward<A4>(a4), nil()				 , nil()			   ,
						   nil()			   , nil()				 , nil()				);	}
	// operator(a,b,c)
	template <typename A1, typename A2, typename A3>
		auto operator ()(A1&& a1, A2&& a2, A3&& a3) const ->typename mySignature<A1, A2, A3>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3),
						   nil()			   , nil()				 , nil()			   ,
						   nil()			   , nil()				 , nil()				);	}
	// operator(a,b)
	template <typename A1, typename A2>
		auto operator ()(A1&& a1, A2&& a2) const ->typename mySignature<A1, A2>::type::result_t
		{	return (*this)(std::forward<A1>(a1), std::forward<A2>(a2), nil()			   ,
						   nil()			   , nil()				 , nil()			   ,
						   nil()			   , nil()				 , nil()				);	}
	// operator(a)
	template <typename A1>
		auto operator ()(A1&& a1) const ->typename mySignature<A1>::type::result_t
		{	return (*this)(std::forward<A1>(a1), nil()				 , nil()			   ,
						   nil()			   , nil()				 , nil()			   ,
						   nil()			   , nil()				 , nil()				);	}
	// operator(void)
	auto operator ()() const ->typename mySignature<>::type::result_t
		{	return (*this)(nil(), nil(), nil(), nil(), nil(), nil(), nil(), nil(), nil());	}
};
//************************************************************************************************
//change from pObj->*  to  Obj.*       pObj->*  を  Obj.* に変換
template<typename T>
	struct p_o			{
		template <typename U>
		static U&& get(U&& x) { return std::forward<U>(x); }
	};
template<typename T>
	struct p_o<T*>		{
		template <typename U>
		static auto get(U&& p) ->decltype(*p) { return *p; }
	};
//------------------------------------------------------------------------------------------------
// execute with parameters
//member
template <typename S>
	struct executer<S, mem_c>		{
		typedef typename S::fn_t		fn_t;
		typedef typename S::call_t		call_t;
		typedef typename S::result_t	result_t;
		typedef p_o<call_t>				p_ot;
		typedef typename S::p1_t		p1_t;
		result_t exec(fn_t&& fn, p1_t&& Obj, ...) const
			{	return p_ot::get(std::forward<p1_t>(Obj)).*fn;	}
	};
//member function
template <typename S>
	struct executer<S, memF_c>	{
		typedef typename S::fn_t		fn_t;
		typedef typename S::call_t		call_t;
		typedef typename S::result_t	result_t;
		typedef p_o<call_t>				p_ot;
		typedef typename S::p1_t		p1_t;
			typedef typename S::p2_t		p2_t;
				typedef typename S::p3_t		p3_t;
					typedef typename S::p4_t		p4_t;
						typedef typename S::p5_t		p5_t;
							typedef typename S::p6_t		p6_t;
								typedef typename S::p7_t		p7_t;
									typedef typename S::p8_t		p8_t;
										typedef typename S::p9_t		p9_t;
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, p7_t&& p7, p8_t&& p8, p9_t&& p9) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2), std::forward<p3_t>(p3),
									std::forward<p4_t>(p4), std::forward<p5_t>(p5), std::forward<p6_t>(p6),
									std::forward<p7_t>(p7), std::forward<p8_t>(p8), std::forward<p9_t>(p9));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, p7_t&& p7, p8_t&& p8, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2),
									std::forward<p3_t>(p3), std::forward<p4_t>(p4), std::forward<p5_t>(p5),
									std::forward<p6_t>(p6), std::forward<p7_t>(p7), std::forward<p8_t>(p8));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, p7_t&& p7, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2), std::forward<p3_t>(p3), std::forward<p4_t>(p4),
									std::forward<p5_t>(p5), std::forward<p6_t>(p6), std::forward<p7_t>(p7));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, nil*, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2), std::forward<p3_t>(p3),
									std::forward<p4_t>(p4), std::forward<p5_t>(p5), std::forward<p6_t>(p6));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, nil*, nil*, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2), std::forward<p3_t>(p3),
									std::forward<p4_t>(p4), std::forward<p5_t>(p5));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, p4_t&& p4, nil*, nil*, nil*, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2), std::forward<p3_t>(p3), std::forward<p4_t>(p4));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, p3_t&& p3, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(
									std::forward<p2_t>(p2), std::forward<p3_t>(p3));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, p2_t&& p2, nil*, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)(std::forward<p2_t>(p2));	}
		result_t exec(fn_t&& fn, p1_t&& Obj, nil*, nil*, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return (p_ot::get(std::forward<p1_t>(Obj)).*fn)();	}
	};
//functor
template <typename S>
	struct executer<S, fnc_c>	{
		typedef typename S::result_t	result_t;
		typedef typename S::fn_t	fn_t;
		typedef typename S::p1_t	p1_t;
			typedef typename S::p2_t	p2_t;
				typedef typename S::p3_t	p3_t;
					typedef typename S::p4_t	p4_t;
						typedef typename S::p5_t	p5_t;
							typedef typename S::p6_t	p6_t;
								typedef typename S::p7_t	p7_t;
									typedef typename S::p8_t	p8_t;
										typedef typename S::p9_t	p9_t;
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, p7_t&& p7, p8_t&& p8, p9_t&& p9) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3),
							std::forward<p4_t>(p4), std::forward<p5_t>(p5), std::forward<p6_t>(p6),
							std::forward<p7_t>(p7), std::forward<p8_t>(p8), std::forward<p9_t>(p9));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, p7_t&& p7, p8_t&& p8, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3),
							std::forward<p4_t>(p4), std::forward<p5_t>(p5), std::forward<p6_t>(p6),
							std::forward<p7_t>(p7), std::forward<p8_t>(p8));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, p7_t&& p7, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3),
							std::forward<p4_t>(p4), std::forward<p5_t>(p5), std::forward<p6_t>(p6),
							std::forward<p7_t>(p7));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, p6_t&& p6, nil*, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3),
							std::forward<p4_t>(p4), std::forward<p5_t>(p5), std::forward<p6_t>(p6));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, p4_t&& p4, p5_t&& p5, nil*, nil*, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3),
							std::forward<p4_t>(p4), std::forward<p5_t>(p5));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, p4_t&& p4, nil*, nil*, nil*, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3),
							std::forward<p4_t>(p4));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, p3_t&& p3, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2), std::forward<p3_t>(p3));	}
		result_t exec(fn_t&& fn, p1_t&& p1, p2_t&& p2, nil*, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1), std::forward<p2_t>(p2));	}
		result_t exec(fn_t&& fn, p1_t&& p1, nil*, nil*, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return fn(std::forward<p1_t>(p1));	}
		result_t exec(fn_t&& fn, nil*, nil*, nil*, nil*, nil*, nil*, nil*, nil*, nil*) const
			{	return fn();	}
	};
//************************************************************************************************
	// workaround decltype
template <typename F, typename P1 = nil, typename P2 = nil, typename P3 = nil, typename P4 = nil, typename P5 = nil
					, typename P6 = nil, typename P7 = nil, typename P8 = nil, typename P9 = nil>
	class rbind_type	{
		typedef BindOf<F, nil , 0>		B0;
			typedef BindOf<P1, B0, 1>		B1;
				typedef BindOf<P2, B1, 2>		B2;
					typedef BindOf<P3, B2, 3>		B3;
						typedef BindOf<P4, B3, 4>		B4;
							typedef BindOf<P5, B4, 5>		B5;
								typedef BindOf<P6, B5, 6>		B6;
									typedef BindOf<P7, B6, 7>		B7;
										typedef BindOf<P8, B7, 8>		B8;
											typedef BindOf<P9, B8, 9>		B9;
		static const int N = std::is_same<P1, nil>::value ? 0:
								std::is_same<P2, nil>::value ? 1:
									std::is_same<P3, nil>::value ? 2:
										std::is_same<P4, nil>::value ? 3:
											std::is_same<P5, nil>::value ? 4:
												std::is_same<P6, nil>::value ? 5:
													std::is_same<P7, nil>::value ? 6:
														std::is_same<P8, nil>::value ? 7:
															std::is_same<P9, nil>::value ? 8:
																							   9;
	public:
		typedef typename n_th_type<N, B0, B1, B2, B3, B4, B5, B6, B7, B8, B9>::type		type; 
	};
} //namespace my::detail
} //namespace my

//***********************************************************************************************
namespace std {
	template <int N, typename F, typename R>
		struct is_placeholder<my::detail::placeholder_with_F<N, R, F>>	{
			static const int value = N;
		};
#if 0	//#ifdef BOOST_BIND_ARG_HPP_INCLUDED
	template <> struct is_placeholder<boost::arg<1>> : integral_constant<int, 1> { };
	template <> struct is_placeholder<boost::arg<2>> : integral_constant<int, 2> { };
	template <> struct is_placeholder<boost::arg<3>> : integral_constant<int, 3> { };
	template <> struct is_placeholder<boost::arg<4>> : integral_constant<int, 4> { };
	template <> struct is_placeholder<boost::arg<5>> : integral_constant<int, 5> { };
	template <> struct is_placeholder<boost::arg<6>> : integral_constant<int, 6> { };
	template <> struct is_placeholder<boost::arg<7>> : integral_constant<int, 7> { };
	template <> struct is_placeholder<boost::arg<8>> : integral_constant<int, 8> { };
	template <> struct is_placeholder<boost::arg<9>> : integral_constant<int, 9> { };
#endif
}	//namespace std

namespace my	{
	namespace detail	{
	//placeholders
	template <int N>
		struct placeholders_deploy	{
			static placeholder_with_F<N, void, void> static_N;
		};
		template <int N>	//static member
		placeholder_with_F<N, void, void> placeholders_deploy<N>::static_N =
			placeholder_with_F<N, void, void>();
	}	//namespace detail

//pre-defined placeholders _1st, _2nd, _3rd, _4th, ...      定義済みプレースホルダ 
	namespace placeholders	{
		namespace {
			detail::placeholder_with_F<1, void, void>&	_1st = detail::placeholders_deploy<1>::static_N;
			detail::placeholder_with_F<2, void, void>&	_2nd = detail::placeholders_deploy<2>::static_N;
			detail::placeholder_with_F<3, void, void>&	_3rd = detail::placeholders_deploy<3>::static_N;
			detail::placeholder_with_F<4, void, void>&	_4th = detail::placeholders_deploy<4>::static_N;
			detail::placeholder_with_F<5, void, void>&	_5th = detail::placeholders_deploy<5>::static_N;
			detail::placeholder_with_F<6, void, void>&	_6th = detail::placeholders_deploy<6>::static_N;
			detail::placeholder_with_F<7, void, void>&	_7th = detail::placeholders_deploy<7>::static_N;
			detail::placeholder_with_F<8, void, void>&	_8th = detail::placeholders_deploy<8>::static_N;
			detail::placeholder_with_F<9, void, void>&	_9th = detail::placeholders_deploy<9>::static_N;
		}
	}	// my::placeholders
//user interfade / rbind functions       ユーザが使う rbind関数
template <typename F>
	auto rbind(F&& f)-> typename detail::rbind_type<F>::type
	{	return detail::BindOf<F>(std::forward<F>(f));	}

template <typename F, typename P1>
	auto rbind(F&& f, P1&& p1)-> typename detail::rbind_type<F, P1>::type
	{	return typename detail::rbind_type<F, P1>::type(std::forward<P1>(p1), rbind(std::forward<F>(f)));	}

template <typename F, typename P1, typename P2>
	auto rbind(F&& f, P1&& p1, P2&& p2)-> typename detail::rbind_type<F, P1, P2>::type
	{	return typename detail::rbind_type<F, P1, P2>::type(
			std::forward<P2>(p2),
				rbind(std::forward<F>(f), std::forward<P1>(p1)));	}

template <typename F, typename P1, typename P2, typename P3>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3)-> typename detail::rbind_type<F, P1, P2, P3>::type
	{	return typename detail::rbind_type<F, P1, P2, P3>::type(
			std::forward<P3>(p3),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2)));	}

template <typename F, typename P1, typename P2, typename P3, typename P4>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3, P4&& p4)-> typename detail::rbind_type<F, P1, P2, P3, P4>::type
	{	return typename detail::rbind_type<F, P1, P2, P3, P4>::type(
			std::forward<P4>(p4),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3)));	}

template <typename F, typename P1, typename P2, typename P3, typename P4, typename P5>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5)
		-> typename detail::rbind_type<F, P1, P2, P3, P4, P5>::type
	{	return typename detail::rbind_type<F, P1, P2, P3, P4, P5>::type(
			std::forward<P5>(p5),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2),
					std::forward<P3>(p3), std::forward<P4>(p4)));	}

template <typename F, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6)
		-> typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6>::type
	{	return typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6>::type(
			std::forward<P6>(p6),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2),
					std::forward<P3>(p3), std::forward<P4>(p4), std::forward<P5>(p5)));	}

template <typename F, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7)
		-> typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6, P7>::type
	{	return typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6, P7>::type(
			std::forward<P7>(p7),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2),
					std::forward<P3>(p3), std::forward<P4>(p4), std::forward<P5>(p5), std::forward<P6>(p6)));	}

template <typename F, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7, P8&& p8)
		-> typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6, P7, P8>::type
	{	return typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6, P7, P8>::type(
			std::forward<P8>(p8),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3),
					std::forward<P4>(p4), std::forward<P5>(p5), std::forward<P6>(p6), std::forward<P7>(p7)));	}

template <typename F, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
	auto rbind(F&& f, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7, P8&& p8, P9&& p9)
		-> typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6, P7, P8, P9>::type
	{	return typename detail::rbind_type<F, P1, P2, P3, P4, P5, P6, P7, P8, P9>::type(
			std::forward<P9>(p9),
				rbind(std::forward<F>(f), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3),
					std::forward<P4>(p4), std::forward<P5>(p5), std::forward<P6>(p6), std::forward<P7>(p7),
						std::forward<P8>(p8))) ;	}
} //namespace my
