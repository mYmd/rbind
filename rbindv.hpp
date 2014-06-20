//rbindv header
//Copyright (c) 2014 Masahiko Yamada

#pragma once
#include <functional>
#include <tuple>

namespace my	{
namespace detail	{

	struct nil	{	};

	template<size_t... indice>
	struct index_tuple	{	};

	template <size_t first, size_t last, class result = index_tuple<>, bool flag = (first >= last)>
	struct index_range
		{	typedef result type;	};

	template <size_t first, size_t last, size_t... indice>
	struct index_range<first, last, index_tuple<indice...>, false> : index_range<first + 1, last, index_tuple<indice..., first>>
		{	};

	//std::remove_reference && std::remove_cv
	template<typename T>
	struct remove_ref_cv	{
		typedef typename std::remove_reference<typename std::remove_cv<T>::type>::type	type;
	};
	// type compair without const/volatile and reference   cv属性と参照属性を外して型の同一性を比較
	template<typename T, typename U>
		struct is_same_ignoring_cv_ref :
			std::is_same<typename remove_ref_cv<T>::type, typename remove_ref_cv<U>::type>	{	};

	//--------------------------------------------------
	template<typename Head, typename... Tail>
	struct tuple_limit	{
		static const size_t value = is_same_ignoring_cv_ref<Head, nil>::value?
												0								:
												1 + tuple_limit<Tail...>::value	;
	};

	template<typename Head>
	struct tuple_limit<Head>	{
		static const size_t value = is_same_ignoring_cv_ref<Head, nil>::value? 0: 1;
	};

	template<typename... Params>
	struct tuple_limit<std::tuple<Params...>>	{
		static const size_t value = tuple_limit<Params...>::value;
	};

	//type of invoke    呼び出しの型 ===================================
	//obj.*, pobj->*
	struct mem_c	{	};
	//obj.*(), pobj->*()
	struct memF_c	{	};
	//functor    ファンクタ型（フリー関数、ラムダ）
	struct fnc_c	{	};
	//==================================================================
	//pair <type of invoke, return type>    呼び出しの型と戻り値型のペア
	template <typename C, typename R, size_t N>
	struct cr_pair	{
		typedef C call_type;
		typedef R result_type;
		static const size_t value = N;
	};
	//==================================================================

	//parameter buffer    パラメータバッファ
	template <typename T>
	struct param_buf	{
		mutable T		val;
		typedef T		type;
		T& get() const
		{	return val;	}
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

//placeholder ===========================================================================
	//yield     _1st.yield<T>(functor)    値を評価するplaceholder
	template <int N, typename R, typename F>
	struct placeholder_with_F : param_buf<F>	{
		placeholder_with_F(F&& f) : param_buf<F>(std::forward<F>(f))		{ }
		template <typename V>
			R eval(V&& v) const	{ return (param_buf<F>::get())(std::forward<V>(v)); }
	};
	//yield    _2nd.yield<T>()    statc_castするplaceholder
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
		//yield
		template <typename R, typename F>
			auto yield(F&& f) const ->placeholder_with_F<N, R, F>
			{ return placeholder_with_F<N, R, F>(std::forward<F>(f)); }
		template <typename R>
			auto yield(void) const ->placeholder_with_F<N, R, void>
			{ return placeholder_with_F<N, R, void>(); }
	};

	//convet from placeholder to argument    placeholderから実引数に変換 ====================
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
	//yield with a functor    ファンクタからyieldされた
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
	//---------------------------------------------------------------------------------
	//handle a type as a raw ponter    スマートポインタ等を生ポインタと同様に扱うための変換 =======
	//T:int => nil*,  int* => int*,  smart_ptr<int> => int*
	template <typename T>
	class raw_ptr_type	{
		static T getT();
		template <typename U>
		static std::true_type test1(U&& u, decltype(*u, 0) = 0);
		static std::false_type	test1(...);
		typedef decltype(test1(getT()))		T1;
		typedef typename std::conditional<T1::value , T, nil*>::type  psp_type; 
		static psp_type getPSP();
	public:
		typedef decltype(&*getPSP())	type;		//T:int=>nil*, int*=>int*, s_ptr<int>=>int*
	};

	//************************************************************************************************
	//arguments    パラメータ
	template <typename P>
	struct ParamOf : param_buf<P>	{
		typedef param_buf<P> base;
		typedef P param_t;
		typedef typename remove_ref_cv<P>::type		p_h;
		static const int placeholder = std::is_placeholder<p_h>::value;
		ParamOf(P&& p) : base(std::forward<P>(p))	{	}
		ParamOf(ParamOf<P>&& p) : base(std::forward<base>(p))	{	}
		ParamOf(const ParamOf<P>& p) : base(p)	{	}
	};
	//----------------------------------------------------------------------------
	template <typename P0, typename Params, size_t N, bool B>
	class type_shift_1	{
		typedef typename remove_ref_cv<typename P0::param_t>::type		ph;
		typedef typename std::tuple_element<N-1, Params>::type			type_a;
		typedef typename parameter_evaluate<ph>::template eval<type_a>	vtype;
	public:
		typedef typename vtype::type									type;
		static type get(const P0& p, Params&& params)
		{
			return vtype::get(p.get(), std::get<N-1>(std::forward<Params>(params)));
		}
	};

	template <typename P0, typename Params>
	struct type_shift_1<P0, Params, 0, true>	{
		typedef typename P0::param_t&		type;	//ここ
		static type get(const P0& p, Params&& params)
		{
			return p.get();
		}
	};

	template <typename P0, typename Params, size_t N>
	class type_shift_1<P0, Params, N, false>		{
		typedef typename remove_ref_cv<typename P0::param_t>::type		ph;
		typedef typename parameter_evaluate<ph>::template eval<nil>		vtype;
	public:
		typedef typename vtype::type									type;
		static type get(const P0& p, Params&& params)
		{
			return vtype::get(p.get(), nil());
		}
	};
	//----------------------------------------------------------------------------
	template <size_t N, typename Params1, typename Params2>
	class get_and_convert_result	{
		typedef typename std::tuple_element<N, Params1>::type		P0;
		static const int	placeholder	= P0::placeholder;
		static const bool small = (placeholder <= std::tuple_size<Params2>::value);
		typedef type_shift_1<P0, Params2, placeholder, small>	alt_type;
	public:
		typedef typename alt_type::type		result_type;
		static result_type get(const Params1& params1, Params2&& params2)
		{
			return alt_type::get(std::get<N>(params1), std::forward<Params2>(params2));
		}
	};

	template<size_t N, typename Params1, typename Params2>
	auto get_and_convert(const Params1& params1, Params2&& params2)
		->typename get_and_convert_result<N, Params1, Params2>::result_type
	{
		typedef get_and_convert_result<N, Params1, Params2>		convert_result;
		return convert_result::get(params1, std::forward<Params2>(params2));
	}

	//=================================================================================================
	template<typename TPL>
	class invokeType	{
		template<int N>
			static auto get()->typename std::tuple_element<N, TPL>::type;
		template<int N>
			static auto getp()->typename raw_ptr_type<typename std::tuple_element<N, TPL>::type>::type;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0&&					t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T0>(t0)(get<indice1>()...), 1) = 0	)
			->cr_pair<	fnc_c	,
						decltype(std::forward<T0>(t0)(get<indice1>()...))	,
						1		>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1>(t1).*t0)(get<indice2>()...), 1) = 0	)
			->cr_pair<	memF_c	,
						decltype((std::forward<T1>(t1).*t0)(get<indice2>()...))	,
						2		>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1P>(t1p)->*t0)(get<indice2>()...), 1) = 0	)
			->cr_pair<	memF_c*		,
						decltype((std::forward<T1P>(t1p)->*t0)(get<indice2>()...))	,
						2			>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1>(t1).*t0, 1) = 0	)
			->cr_pair<	mem_c	,
						decltype(std::forward<T1>(t1).*t0)	,
						2		>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1P>(t1p)->*t0, 1) = 0	)
			->cr_pair<	mem_c*	,
						decltype(std::forward<T1P>(t1p)->*t0)	,
						2		>;
		//
		static const size_t ParamSize = tuple_limit<TPL>::value;
		static typename index_range<1, ParamSize>::type		index_tuple_1();
		static typename index_range<2, ParamSize>::type		index_tuple_2();
		typedef decltype(test(	index_tuple_1()		,
								index_tuple_2()		,
								get<0>()			,
								get<1>()			,
								getp<1>()			)	)	cr_type;
	public:
		typedef typename cr_type::call_type									call_type;
		typedef typename cr_type::result_type								result_type;
		typedef typename index_range<0, ParamSize - cr_type::value>::type	actual_indice;
	};

	//************************************************************************************************
	// execute with parameters
	template <typename R, typename T, typename A> struct executer	{
	};

	//member
	template <typename R, size_t... indice>
	struct executer<R, mem_c, index_tuple<indice...>>		{
		template <typename M, typename Obj>
		R exec(M mem, Obj&& obj) const
		{	return (std::forward<Obj>(obj)).*mem;	}
	};
	template <typename R, size_t... indice>
	struct executer<R, mem_c*, index_tuple<indice...>>		{
		template <typename M, typename pObj>
		R exec(M mem, pObj&& pobj) const
		{	return (*std::forward<pObj>(pobj)).*mem;	}
	};
	//member function
	template <typename R, size_t... indice>
	struct executer<R, memF_c, index_tuple<indice...>>	{
		template <typename M, typename Obj, typename... Params>
		R exec(M mem, Obj&& obj, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return ((std::forward<Obj>(obj)).*mem)(std::get<indice>(vt)...);
		}
	};
	template <typename R, size_t... indice>
	struct executer<R, memF_c*, index_tuple<indice...>>	{
		template <typename M, typename pObj, typename... Params>
		R exec(M mem, pObj&& pobj, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return ((*std::forward<pObj>(pobj)).*mem)(std::get<indice>(vt)...);
		}
	};
	//functor
	template <typename R, size_t... indice>
	struct executer<R, fnc_c, index_tuple<indice...>>	{
		template <typename F, typename... Params>
		R exec(F&& f, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return (std::forward<F>(f))(std::get<indice>(vt)...);
		}
	};

	//***************************************************************************************
	//main class    本体 ====================================================================
	template <typename... Vars>
	class	BindOf	{
		static const int	N =  sizeof...(Vars);
		typedef typename index_range<0, N>::type	index_tuple_type;
		typedef std::tuple<ParamOf<Vars>...>		Params1;
		Params1										params1;
		//-----------------------------------------------
		template<typename Params2T, typename D>
		struct invoke_type_i;
		template<typename Params2T, size_t... indice>
		struct invoke_type_i<Params2T, index_tuple<indice...>>	{
			static const Params1&	get1();
			static Params2T			get2();
			typedef decltype(std::forward_as_tuple(
								my::detail::template get_and_convert<indice>(get1(), get2())...)
							)			ParamTuple;
			typedef typename my::detail::invokeType<ParamTuple>			invoke_type;
			typedef typename invoke_type::actual_indice					actual_indice;
			typedef typename invoke_type::call_type						call_type;
			typedef typename invoke_type::result_type					result_type;
			typedef executer<result_type, call_type, actual_indice>		Executer_t;
		};
		//
		template<typename Params2T, size_t... indice>
		auto call_imple(Params2T&& params2, index_tuple<indice...> dummy) const
			->typename invoke_type_i<Params2T, index_tuple<indice...>>::result_type
		{
			typename invoke_type_i<Params2T, index_tuple<indice...>>::Executer_t	Executer;
			return 
				Executer.exec(
					get_and_convert<indice>(params1, std::forward<Params2T>(params2))...
				);
		}
	public:
		BindOf(Vars&&... vars) : params1(std::forward<Vars>(vars)...)	{	}
		//
		template <typename... Params2>
		auto operator ()(Params2&&... params2) const
			->typename invoke_type_i<std::tuple<Params2...>, index_tuple_type>::result_type
		{
			return call_imple(	std::forward_as_tuple(std::forward<Params2>(params2)...),
								index_tuple_type()
							);
		}
	};
} //namespace my::detail
} //namespace my

//***********************************************************************************************
namespace std {
	template <int N, typename F, typename R>
		struct is_placeholder<my::detail::placeholder_with_F<N, R, F>>	{
			static const int value = N;
		};
/*#if 0	//#ifdef BOOST_BIND_ARG_HPP_INCLUDED
	template <> struct is_placeholder<boost::arg<1>> : integral_constant<int, 1> { };
	template <> struct is_placeholder<boost::arg<2>> : integral_constant<int, 2> { };
	template <> struct is_placeholder<boost::arg<3>> : integral_constant<int, 3> { };
	template <> struct is_placeholder<boost::arg<4>> : integral_constant<int, 4> { };
	template <> struct is_placeholder<boost::arg<5>> : integral_constant<int, 5> { };
	template <> struct is_placeholder<boost::arg<6>> : integral_constant<int, 6> { };
	template <> struct is_placeholder<boost::arg<7>> : integral_constant<int, 7> { };
	template <> struct is_placeholder<boost::arg<8>> : integral_constant<int, 8> { };
	template <> struct is_placeholder<boost::arg<9>> : integral_constant<int, 9> { };
#endif*/
}	//namespace std

//***********************************************************************************************
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
			detail::placeholder_with_F< 1, void, void>&  _1st = detail::placeholders_deploy< 1>::static_N;
			detail::placeholder_with_F< 2, void, void>&  _2nd = detail::placeholders_deploy< 2>::static_N;
			detail::placeholder_with_F< 3, void, void>&  _3rd = detail::placeholders_deploy< 3>::static_N;
			detail::placeholder_with_F< 4, void, void>&  _4th = detail::placeholders_deploy< 4>::static_N;
			detail::placeholder_with_F< 5, void, void>&  _5th = detail::placeholders_deploy< 5>::static_N;
			detail::placeholder_with_F< 6, void, void>&  _6th = detail::placeholders_deploy< 6>::static_N;
			detail::placeholder_with_F< 7, void, void>&  _7th = detail::placeholders_deploy< 7>::static_N;
			detail::placeholder_with_F< 8, void, void>&  _8th = detail::placeholders_deploy< 8>::static_N;
			detail::placeholder_with_F< 9, void, void>&  _9th = detail::placeholders_deploy< 9>::static_N;
			detail::placeholder_with_F<10, void, void>& _10th = detail::placeholders_deploy<10>::static_N;
			detail::placeholder_with_F<11, void, void>& _11th = detail::placeholders_deploy<11>::static_N;
			detail::placeholder_with_F<12, void, void>& _12th = detail::placeholders_deploy<12>::static_N;
			detail::placeholder_with_F<13, void, void>& _13th = detail::placeholders_deploy<13>::static_N;
			detail::placeholder_with_F<14, void, void>& _14th = detail::placeholders_deploy<14>::static_N;
			detail::placeholder_with_F<15, void, void>& _15th = detail::placeholders_deploy<15>::static_N;
			detail::placeholder_with_F<16, void, void>& _16th = detail::placeholders_deploy<16>::static_N;
			detail::placeholder_with_F<17, void, void>& _17th = detail::placeholders_deploy<17>::static_N;
			detail::placeholder_with_F<18, void, void>& _18th = detail::placeholders_deploy<18>::static_N;
			detail::placeholder_with_F<19, void, void>& _19th = detail::placeholders_deploy<19>::static_N;
			detail::placeholder_with_F<20, void, void>& _20th = detail::placeholders_deploy<20>::static_N;
			detail::placeholder_with_F<21, void, void>& _21th = detail::placeholders_deploy<21>::static_N;
		}
	}	// my::placeholders



	//************************************************************************
	//user interfade / rbind functions       ユーザが使う rbind関数
	template <typename... Vars>
	detail::BindOf<Vars...> rbind(Vars&&... vars)
	{
		return detail::BindOf<Vars...>(std::forward<Vars>(vars)...);
	}

} //namespace my
