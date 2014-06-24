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

	template <size_t first, size_t last, typename result = index_tuple<>, bool flag = (first >= last)>
	struct index_range
		{	typedef result type;	};

	template <size_t first, size_t last, size_t... indice>
		struct	index_range<first, last, index_tuple<indice...>, false> :
					index_range<first + 1, last, index_tuple<indice..., first>>		{	};

	//std::remove_reference && std::remove_cv
	template<typename T>
	struct remove_ref_cv	{
		typedef typename std::remove_reference<typename std::remove_cv<T>::type>::type	type;
	};

	// type compair without const/volatile and reference   cv属性と参照属性を外して型の同一性を比較
	template<typename T, typename U>
		struct is_same_ignoring_cv_ref :
			std::is_same<typename remove_ref_cv<T>::type, typename remove_ref_cv<U>::type>	{	};
	//--------------------------------------------------------------------
	//	[type0, type1, type2, ..., typeM, nil) ... typeN ,,,
	//       0,     1,     2, ....        M+1
	template<typename Head, typename... Tail>
	struct tuple_limit	{
		static const size_t value = is_same_ignoring_cv_ref<Head, nil>::value?
												0								:
												1 + tuple_limit<Tail...>::value	;
	};
	//
	template<typename Head>
	struct tuple_limit<Head>	{
		static const size_t value = is_same_ignoring_cv_ref<Head, nil>::value? 0: 1;
	};
	//
	template<typename... Params>
	struct tuple_limit<std::tuple<Params...>>	{
		static const size_t value = tuple_limit<Params...>::value;
	};
	//--------------------------------------------------------------------
	//type of invoke    呼び出しの型
	//obj.*, pobj->*
	struct mem_c	{	};
	//obj.*(), pobj->*()
	struct memF_c	{	};
	//functor    ファンクタ型（フリー関数、ラムダ）
	struct fnc_c	{	};
	//==================================================================
	//tuple<type of invoke, return type, sizeof... parameters>    呼び出しの型
	template <typename C, typename R, size_t N>
	struct sig_type		{
		typedef C	call_type;
		typedef R	result_type;
		static const size_t value = N;
	};
	//==================================================================

	//parameter buffer    パラメータバッファ
	template <typename T>
	struct param_buf	{
		mutable T		val;
		typedef T		type;
		param_buf(T&& t) : val(std::forward<T>(t))	{	}
		T& get() const		{	return val;	}
	};
	//		ref
	template <typename T>
	struct param_buf<T&>	{
		T*				pval;
		typedef T&		type;
		param_buf(T& t) : pval(&t)					{	}
		T& get() const		{ return *pval; }
	};
	//		rref
	template <typename T>
	struct param_buf<T&&>	{
		mutable T		val;
		typedef T		type;
		param_buf(T&& t) : val(std::forward<T>(t))	{	}
		T& get() const		{	return val;	}
	};

	//placeholder =======================================================================
	//yield     _1st.yield<T>(functor)    値を評価するplaceholder
	template <size_t N, typename R, typename F>
	struct placeholder_with_F : param_buf<F>	{
		placeholder_with_F(F&& f) : param_buf<F>(std::forward<F>(f))	{	}
		template <typename V>
			R eval(V&& v) const	{ return (param_buf<F>::get())(std::forward<V>(v)); }
	};

	//yield    _2nd.yield<T>()    statc_castするplaceholder
	template <size_t N, typename R>
	struct placeholder_with_F<N, R, void>		{	};

	//with default parameter    デフォルト引数を付与されたplaceholder
	template <size_t N, typename T>
	struct placeholder_with_F<N, void, T> : param_buf<T>	{
		placeholder_with_F(T&& t) : param_buf<T>(std::forward<T>(t))	{	}
	};

	//basic placeholder    基本のplaceholder
	template <size_t N>
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

	template <size_t N>
	using plhdr_t = placeholder_with_F<N, void, void>;

	//until(_9th) => [_1st, _2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
	template <size_t N>
	auto until(const plhdr_t<N>& ) ->typename index_range<1, N+1>::type
	{
		return typename index_range<1, N+1>::type{};
	}

	//range(_2nd, _9th) => [_2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
	template <size_t M, size_t N>
	auto range(const plhdr_t<M>& , const plhdr_t<N>& ) ->typename index_range<M, N+1>::type
	{
		return typename index_range<M, N+1>::type{};
	}

	//=======================================================================================
	//convet from placeholder to argument    placeholderから実引数に変換 ====================
	//not my placeholder    my::detail::placeholder以外
	template <typename T>
	struct parameter_evaluate	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(T& , V&& v)
				{	return std::forward<V>(v);	}
			};
	};

	//yield with a functor    ファンクタからyieldされた
	template <size_t N, typename F, typename R>
	struct parameter_evaluate<placeholder_with_F<N, R, F>>	{
		template <typename V>
			struct eval	{
				typedef R	type;
				static R get(placeholder_with_F<N, R, F>& t, V&& v)
				{	return t.eval(std::forward<V>(v));	}
			};
	};

	//static_cast
	template <size_t N, typename R>
	struct parameter_evaluate<placeholder_with_F<N, R, void>>	{
		template <typename V>
			struct eval	{
				typedef R	type;
				static R get(placeholder_with_F<N, R, void>& , V&& v)
				{	return static_cast<R>(std::forward<V>(v));	}
			};
	};

	//with default value    デフォルト値を与えられた
	template <size_t N, typename T>
	struct parameter_evaluate<placeholder_with_F<N, void, T>>	{
		template <typename V, typename W = typename remove_ref_cv<V>::type>
			struct eval	{			//if argument assigned    実引数あり
				typedef V	type;
				static V&& get(placeholder_with_F<N, void, T>& , V&& v)
				{	return std::forward<V>(v);	}
			};
		template <typename V>
			struct eval<V, nil>	{	//without argument    実引数なし → デフォルト値
				typedef T&	type;
				static T& get(placeholder_with_F<N, void, T>& t, V&& )
				{	return t.get();	}
			};
	};

	//basic    基本
	template <size_t N>
	struct parameter_evaluate<placeholder_with_F<N, void, void>>	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(placeholder_with_F<N, void, void>& , V&& v)
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
		using T1       = decltype(test1(getT()));
		using psp_type = typename std::conditional<T1::value , T, nil*>::type; 
		static psp_type getPSP();
	public:
		typedef decltype(&*getPSP())	type;		//T:int=>nil*, int*=>int*, s_ptr<int>=>int*
	};

	//************************************************************************************************
	//arguments    パラメータ
	template <typename P>
	struct ParamOf : param_buf<P>	{
		using base = param_buf<P>;
		using p_h  = typename remove_ref_cv<P>::type;
		typedef P	param_t;
		static const size_t placeholder = std::is_placeholder<p_h>::value;
		ParamOf(P&& p) : base(std::forward<P>(p))	{	}
	};
	//----------------------------------------------------------------------------
	template <typename P0, typename Params, size_t N, bool B>
	class type_alternative	{
		using ph     = typename remove_ref_cv<typename P0::param_t>::type;
		using type_a = typename std::tuple_element<N-1, Params>::type;
		using vtype  = typename parameter_evaluate<ph>::template eval<type_a>;
	public:
		typedef typename vtype::type			type;
		static type get(const P0& p, Params&& params)
		{
			return vtype::get(p.get(), std::get<N-1>(std::forward<Params>(params)));
		}
	};

	template <typename P0, typename Params>
	struct type_alternative<P0, Params, 0, true>	{
		typedef typename P0::param_t&			type;	//ここ
		static type get(const P0& p, Params&& )
		{
			return p.get();
		}
	};

	template <typename P0, typename Params, size_t N>
	class type_alternative<P0, Params, N, false>		{
		using ph    = typename remove_ref_cv<typename P0::param_t>::type;
		using vtype = typename parameter_evaluate<ph>::template eval<nil>;
	public:
		typedef typename vtype::type			type;
		static type get(const P0& p, Params&& )
		{
			return vtype::get(p.get(), nil());
		}
	};
	//----------------------------------------------------------------------------
	template <size_t N, typename Params1, typename Params2>
	class alternative_result	{
		using P0 = typename std::tuple_element<N, Params1>::type;
		static const size_t	placeholder	= P0::placeholder;
		static const bool small = (placeholder <= std::tuple_size<Params2>::value);
		using alt_type = type_alternative<P0, Params2, placeholder, small>;
	public:
		typedef typename alt_type::type		result_type;
		static result_type get(const Params1& params1, Params2&& params2)
		{
			return alt_type::get(std::get<N>(params1), std::forward<Params2>(params2));
		}
	};

	template<size_t N, typename Params1, typename Params2>
	auto get_and_convert(const Params1& params1, Params2&& params2)
		->typename alternative_result<N, Params1, Params2>::result_type
	{
		using convert_result = alternative_result<N, Params1, Params2>		;
		return convert_result::get(params1, std::forward<Params2>(params2));
	}

	//=================================================================================================
	template<typename TPL>
	class invokeType	{		//	SFINAE
		static const size_t ParamSize = tuple_limit<TPL>::value;
		template<size_t N>
			static auto get()->typename std::tuple_element<N, TPL>::type;
		template<size_t N>
			static auto getp()->typename raw_ptr_type<typename std::tuple_element<N, TPL>::type>::type;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0&&					t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T0>(t0)(get<indice1>()...), 1) = 0	)
			->sig_type<	fnc_c	,
						decltype(std::forward<T0>(t0)(get<indice1>()...))	,
						ParamSize - 1	>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1>(t1).*t0)(get<indice2>()...), 1) = 0	)
			->sig_type<	memF_c	,
						decltype((std::forward<T1>(t1).*t0)(get<indice2>()...))	,
						ParamSize - 2	>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1P>(t1p)->*t0)(get<indice2>()...), 1) = 0	)
			->sig_type<	memF_c*		,
						decltype((std::forward<T1P>(t1p)->*t0)(get<indice2>()...))	,
						ParamSize - 2	>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1>(t1).*t0, 1) = 0	)
			->sig_type<	mem_c	,
						decltype(std::forward<T1>(t1).*t0)	,
						ParamSize - 2	>;
		//
		template<size_t... indice1, size_t... indice2, typename T0, typename T1, typename T1P>
		static auto test(	index_tuple<indice1...>		,
							index_tuple<indice2...>		,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1P>(t1p)->*t0, 1) = 0	)
			->sig_type<	mem_c*	,
						decltype(std::forward<T1P>(t1p)->*t0)	,
						ParamSize - 2	>;
		//
		static typename index_range<1, ParamSize>::type		index_tuple_1();
		static typename index_range<2, ParamSize>::type		index_tuple_2();
		using sig_t = decltype(test(	index_tuple_1()		,
										index_tuple_2()		,
										get<0>()			,
										get<1>()			,
										getp<1>()			)	)	;
	public:
		typedef typename sig_t::call_type						call_type;
		typedef typename sig_t::result_type						result_type;
		typedef typename index_range<0, sig_t::value>::type		actual_indice;
	};

	//************************************************************************************************
	// execute with parameters
	template <typename R, typename T, typename A> struct executer;

	//member
	template <typename R, size_t... indice>
	struct executer<R, mem_c, index_tuple<indice...>>		{
		template <typename M, typename Obj>
		R exec(M mem, Obj&& obj) const
		{	return (std::forward<Obj>(obj)).*mem;	}
	};
	//----
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
	//----
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
		static const size_t	N =  sizeof...(Vars);
		using index_tuple_type = typename index_range<0, N>::type;
		using Params1 = std::tuple<ParamOf<Vars>...>;
		Params1										params1;
		//-----------------------------------------------
		template<typename Params2T, typename D>		struct invoke_type_i;
		//------------
		template<typename Params2T, size_t... indice>
		struct invoke_type_i<Params2T, index_tuple<indice...>>	{
			static const Params1&	get1();
			static Params2T			get2();
			using ParamTuple = decltype(std::forward_as_tuple(
									my::detail::template get_and_convert<indice>(get1(), get2())...)
										);
			using invoke_type   = typename my::detail::invokeType<ParamTuple>;
			using actual_indice = typename invoke_type::actual_indice;
			using call_type     = typename invoke_type::call_type;
			using result_type   = typename invoke_type::result_type;
			using Executer_t    = executer<result_type, call_type, actual_indice>;
		};
		//
		template<typename Params2T, size_t... indice>
		auto call_imple(Params2T&& params2, index_tuple<indice...> ) const
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

	//*******************************************************************
	template <typename T>
	auto granulate(T&& t)->std::tuple<T&&>
	{
		return std::forward_as_tuple(std::forward<T>(t));
	}

	template <size_t... indice>
	auto granulate(index_tuple<indice...>&& )
		-> std::tuple<placeholder_with_F<indice, void, void>...>
	{
		return std::tuple<placeholder_with_F<indice, void, void>...>();
	};
	
	template <typename... T>
	auto untie_vars(T&&... t)->decltype(std::tuple_cat(granulate(std::forward<T>(t))...))
	{
		return std::tuple_cat(granulate(std::forward<T>(t))...);
	}
	//-------------------------------------------------
	template <typename... Vars, size_t... indice>
	auto rbind_imple2(std::tuple<Vars...>&& vars, index_tuple<indice...> )->BindOf<Vars...>
	{
		using B = BindOf<Vars...>;
		return B(std::get<indice>(std::forward<std::tuple<Vars...>>(vars))...);
	}

	template <typename... Vars>
	auto rbind_imple(std::tuple<Vars...>&& vars)->BindOf<Vars...>
	{
		using index = typename index_range<0, sizeof...(Vars)>::type;
		return rbind_imple2(std::forward<std::tuple<Vars...>>(vars), index());
	}

} //namespace my::detail
} //namespace my

//***********************************************************************************************
namespace std {
	template <size_t N, typename F, typename R>
		struct is_placeholder<my::detail::placeholder_with_F<N, R, F>>	{
			static const size_t value = N;
		};
/*#if 0	//#ifdef BOOST_BIND_ARG_HPP_INCLUDED
	template <> struct is_placeholder<boost::arg<1>> : integral_constant<int, 1> { };
	...
#endif*/
}	//namespace std

//***********************************************************************************************
namespace my	{
	//predefined placeholders _1st, _2nd, _3rd, _4th, ...      定義済みプレースホルダ 
	namespace placeholders	{
		namespace {
			detail::placeholder_with_F< 1, void, void>  _1st;
			detail::placeholder_with_F< 2, void, void>  _2nd;
			detail::placeholder_with_F< 3, void, void>  _3rd;
			detail::placeholder_with_F< 4, void, void>  _4th;
			detail::placeholder_with_F< 5, void, void>  _5th;
			detail::placeholder_with_F< 6, void, void>  _6th;
			detail::placeholder_with_F< 7, void, void>  _7th;
			detail::placeholder_with_F< 8, void, void>  _8th;
			detail::placeholder_with_F< 9, void, void>  _9th;
			detail::placeholder_with_F<10, void, void> _10th;
			detail::placeholder_with_F<11, void, void> _11th;
			detail::placeholder_with_F<12, void, void> _12th;
			detail::placeholder_with_F<13, void, void> _13th;
			detail::placeholder_with_F<14, void, void> _14th;
			detail::placeholder_with_F<15, void, void> _15th;
			//   ...
		}

		// Alias for placeholder
		template <size_t N>
		using plhdr_t = detail::plhdr_t<N>;

	}	// my::placeholders

	//************************************************************************
	//user interfade / rbind functions       ユーザが使う rbind関数
	template <typename... Vars>
	auto rbind(Vars&&... vars)
		->decltype(detail::rbind_imple(detail::untie_vars(std::forward<Vars>(vars)...)))
	{
		return detail::rbind_imple(detail::untie_vars(std::forward<Vars>(vars)...));
	}

} //namespace my
