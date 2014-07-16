//rbindv header
//Copyright (c) 2014 Masahiko Yamada

#pragma once
#include <functional>

namespace my	{
namespace detail	{

	struct nil	{	};

	//============================================================================
	template <size_t... indices>
	struct indEx_tuple	{
		static constexpr size_t size()	{	return sizeof...(indices);	}
	};

	// linear implementation is sufficient for bind parameters   線形実装で十分
	template <size_t first, size_t last, typename result = indEx_tuple<>, bool flag = (first >= last)>
	struct indEx_range_imple		{
		typedef result type;
	};

	template <size_t first, size_t last, size_t... indices>
	struct	indEx_range_imple<first, last, indEx_tuple<indices...>, false> :
					indEx_range_imple<first + 1, last, indEx_tuple<indices..., first>>		{
	};

	template <size_t first, size_t last>
	using indEx_range = typename indEx_range_imple<first, last>::type;

	//+*******************************************************************************************

	//std::remove_reference && std::remove_cv
	template<typename T>
	class remove_ref_cv	{
		using type2 = typename std::remove_cv<T>::type;
		using type1 = typename std::remove_reference<type2>::type;
	public:
		typedef typename std::remove_cv<type1>::type	type;
	};

	// type compair without const/volatile and reference   cv属性と参照属性を外して型の同一性を比較
	template<typename T, typename U>
		struct is_same_ignoring_cv_ref :
			std::is_same<typename remove_ref_cv<T>::type, typename remove_ref_cv<U>::type>	{	};
	//==================================================================

	//parameter buffer    パラメータバッファ
	template <typename T>
	struct param_buf	{
		mutable T		val;		//std::unique_ptr<T>
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

	//************************************************************************************************
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

	//************************************************************************************************
	//type of invoke    呼び出しの型
	//obj.*, pobj->*
	struct mem_c	{	};
	//obj.*(), pobj->*()
	struct memF_c	{	};
	//functor    ファンクタ型（フリー関数、ラムダ）
	struct fnc_c	{	};
	//--------------------------------------------------------------------
	//tuple<type of invoke, return type, sizeof... parameters>    呼び出しの型
	template <typename C, typename R, size_t N>
	struct sig_type		{
		typedef C	call_type;
		typedef R	result_type;
		static constexpr size_t value = N;
	};
	//---------------------------------------------------------------------------------
	//	[type0, type1, type2, ..., typeM, nil) ... typeN ,,,
	//       0,     1,     2, ....        M+1
	template<typename... Tail>
	struct nil_stop	{
		static constexpr size_t value = 0;
	};

	template<typename Head, typename... Tail>
	struct nil_stop<Head, Tail...>	{
		static constexpr size_t value = is_same_ignoring_cv_ref<Head, nil>::value?
												0								:
												1 + nil_stop<Tail...>::value	;
	};

	template<typename... Params>
	struct nil_stop<std::tuple<Params...>>	{
		static constexpr size_t value = nil_stop<Params...>::value;
	};
	//--------------------------------------------------------------------
	//handle a type as a raw ponter    スマートポインタ等を生ポインタと同様に扱うための変換 =======
	//T:int => nil*,  int* => int*,  smart_ptr<int> => int*
	template <typename T>
	class raw_ptr_type	{
		static T getT();
		template <typename U>
		static std::true_type test1(U&& u, decltype(*u, static_cast<void>(0), 0) = 0);
		static std::false_type	test1(...);
		static constexpr bool flag = decltype(test1(getT()))::value;
		using psp_type = typename std::conditional<flag , T, nil*>::type; 
		static psp_type getPSP();
	public:
		typedef decltype(&*getPSP())	type;		//T:int=>nil*, int*=>int*, s_ptr<int>=>int*
	};
	//--------------------------------------------------------------------
	//determin the type of invocation
	template<typename TPL>
	class invokeType	{		//	SFINAE
		template<size_t N>
			static auto get()->typename std::tuple_element<N, TPL>::type;
		static constexpr size_t ParamSize = nil_stop<TPL>::value;
		//
		template<size_t... indices1, size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_tuple<indices1...>	,
							indEx_tuple<indices2...>	,
							T0&&					t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T0>(t0)(get<indices1>()...), static_cast<void>(0), 1) = 0	)
			->sig_type<	fnc_c	,
						decltype(std::forward<T0>(t0)(get<indices1>()...))	,
						ParamSize - 1	>;
		//
		template<size_t... indices1, size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_tuple<indices1...>	,
							indEx_tuple<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1>(t1).*t0)(get<indices2>()...), static_cast<void>(0), 1) = 0	)
			->sig_type<	memF_c	,
						decltype((std::forward<T1>(t1).*t0)(get<indices2>()...))	,
						ParamSize - 2	>;
		//
		template<size_t... indices1, size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_tuple<indices1...>	,
							indEx_tuple<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1P>(t1p)->*t0)(get<indices2>()...), static_cast<void>(0), 1) = 0	)
			->sig_type<	memF_c*		,
						decltype((std::forward<T1P>(t1p)->*t0)(get<indices2>()...))	,
						ParamSize - 2	>;
		//
		template<size_t... indices1, size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_tuple<indices1...>	,
							indEx_tuple<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1>(t1).*t0, static_cast<void>(0), 1) = 0	)
			->sig_type<	mem_c	,
						decltype(std::forward<T1>(t1).*t0)	,
						ParamSize - 2	>;
		//
		template<size_t... indices1, size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_tuple<indices1...>	,
							indEx_tuple<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1P>(t1p)->*t0, static_cast<void>(0), 1) = 0	)
			->sig_type<	mem_c*	,
						decltype(std::forward<T1P>(t1p)->*t0)	,
						ParamSize - 2	>;
		//
		static auto getp()->typename raw_ptr_type<typename std::tuple_element<1, TPL>::type>::type;
		//
		using sig_t = decltype(test(indEx_range<1, ParamSize>{}	,
									indEx_range<2, ParamSize>{}	,
									get<0>()					,
									get<1>()					,
									getp()						)	)	;
	public:
		typedef typename sig_t::call_type			call_type;
		typedef typename sig_t::result_type			result_type;
		typedef indEx_range<0, sig_t::value>		actual_indice;
	};

	//************************************************************************************************
	// execute
	template <typename R, typename T, typename A> struct executor;

	//member
	template <typename R, size_t... indices>
	struct executor<R, mem_c, indEx_tuple<indices...>>		{
		template <typename M, typename Obj>
		R exec(M mem, Obj&& obj) const
		{	return (std::forward<Obj>(obj)).*mem;	}
	};
	//----
	template <typename R, size_t... indices>
	struct executor<R, mem_c*, indEx_tuple<indices...>>		{
		template <typename M, typename pObj>
		R exec(M mem, pObj&& pobj) const
		{	return (*std::forward<pObj>(pobj)).*mem;	}
	};

	//member function
	template <typename R, size_t... indices>
	struct executor<R, memF_c, indEx_tuple<indices...>>	{
		template <typename M, typename Obj, typename... Params>
		R exec(M mem, Obj&& obj, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return ((std::forward<Obj>(obj)).*mem)(std::get<indices>(vt)...);
		}
	};
	//----
	template <typename R, size_t... indices>
	struct executor<R, memF_c*, indEx_tuple<indices...>>	{
		template <typename M, typename pObj, typename... Params>
		R exec(M mem, pObj&& pobj, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return ((*std::forward<pObj>(pobj)).*mem)(std::get<indices>(vt)...);
		}
	};

	//functor
	template <typename R, size_t... indices>
	struct executor<R, fnc_c, indEx_tuple<indices...>>	{
		template <typename F, typename... Params>
		R exec(F&& f, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return (std::forward<F>(f))(std::get<indices>(vt)...);
		}
	};

	//************************************************************************************************
	//a parameter    パラメータ
	template <typename P>
	struct ParamOf : param_buf<P>	{
		using base = param_buf<P>;
		using p_h  = typename remove_ref_cv<P>::type;
		typedef P	param_t;
		static constexpr size_t placeholder = std::is_placeholder<p_h>::value;
		ParamOf(P&& p) : base(std::forward<P>(p))	{	}
	};
	//----------------------------------------------------------------------------
	template <typename P0, typename Params, size_t N, bool B = true>
	class alt_param	{
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
	struct alt_param<P0, Params, 0, true>	{
		typedef typename P0::param_t&			type;	//ここ
		static type get(const P0& p, Params&& )
		{
			return p.get();
		}
	};

	template <typename P0, typename Params, size_t N>
	class alt_param<P0, Params, N, false>		{		//N is out of range
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
	class param_result	{
		using P0 = typename std::tuple_element<N, Params1>::type;
		static constexpr size_t	placeholder	= P0::placeholder;
		static constexpr bool small = (placeholder <= std::tuple_size<Params2>::value);
		using alt_t = alt_param<P0, Params2, placeholder, small>;
	public:
		typedef typename alt_t::type		result_type;
		static result_type get(const Params1& params1, Params2&& params2)
		{
			return alt_t::get(std::get<N>(params1), std::forward<Params2>(params2));
		}
	};

	template<size_t N, typename Params1, typename Params2>
	auto param_at(const Params1& params1, Params2&& params2)
		->typename param_result<N, Params1, Params2>::result_type
	{
		return param_result<N, Params1, Params2>::get(params1, std::forward<Params2>(params2));
	}

	//***************************************************************************************
	//main class    本体 ====================================================================
	template <typename... Vars>
	class	BindOf	{
		using Params1 = std::tuple<ParamOf<Vars>...>;
		Params1										params1;
		//-----------------------------------------------
		template<typename Params2T, typename D>		struct invoke_type_i;
		//------------
		template<typename Params2T, size_t... indices>
		struct invoke_type_i<Params2T, indEx_tuple<indices...>>	{
			static const Params1&	get1();
			static Params2T			get2();
			using ParamTuple = decltype(std::forward_as_tuple(
									my::detail::template param_at<indices>(get1(), get2())...)
										);
			using invoke_type   = typename my::detail::invokeType<ParamTuple>;
			using actual_indice = typename invoke_type::actual_indice;
			using call_type     = typename invoke_type::call_type;
			using result_type   = typename invoke_type::result_type;
			using Executor_t    = executor<result_type, call_type, actual_indice>;
		};
		//
		template<typename Params2T, size_t... indices>
		auto call_imple(Params2T&& params2, indEx_tuple<indices...> ) const
			->typename invoke_type_i<Params2T, indEx_tuple<indices...>>::result_type
		{
			typename invoke_type_i<Params2T, indEx_tuple<indices...>>::Executor_t	Executor;
			return 
				Executor.exec(
					param_at<indices>(params1, std::forward<Params2T>(params2))...
				);
		}
	public:
		template <size_t... indices>
		BindOf(std::tuple<Vars...>&& vars, indEx_tuple<indices...>) :
				params1(std::get<indices>(std::forward<std::tuple<Vars...>>(vars))...)	{	}
		//
		static constexpr size_t	N =  sizeof...(Vars);
		//
		template <typename... Params2>
		auto operator ()(Params2&&... params2) const
			->typename invoke_type_i<std::tuple<Params2...>, indEx_range<0, N>>::result_type
		{
			return call_imple(	std::forward_as_tuple(std::forward<Params2>(params2)...),
								indEx_range<0, N>()
							);
		}
	};

	//*******************************************************************
	template <typename T>
	auto granulate(T&& t)->std::tuple<T&&>
	{
		return std::forward_as_tuple(std::forward<T>(t));
	}

	template <size_t... indices>
	auto granulate(indEx_tuple<indices...>&& )
		-> std::tuple<placeholder_with_F<indices, void, void>...>
	{
		return std::tuple<placeholder_with_F<indices, void, void>...>();
	};

	template <typename... T>
	auto untie_vars(T&&... t)->decltype(std::tuple_cat(granulate(std::forward<T>(t))...))
	{
		return std::tuple_cat(granulate(std::forward<T>(t))...);
	}
	//-------------------------------------------------
	template <typename... Vars>
	auto rbind_imple(std::tuple<Vars...>&& vars)->BindOf<Vars...>
	{
		using index = indEx_range<0, sizeof...(Vars)>;
		return BindOf<Vars...>(std::forward<std::tuple<Vars...>>(vars), index());
	}

} //namespace my::detail

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

		//until(_9th) => [_1st, _2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
		//this will be converted to tuple of placeholders by granulate function afterward
		// default parameter T is a workaround for visual c++ 2013 CTP
		template <size_t N, typename T = void>
		auto until(const detail::placeholder_with_F<N, T, T>& ) ->detail::indEx_range<1, N+1>
		{
			return detail::indEx_range<1, N+1>{};
		}

		//range(_2nd, _9th) => [_2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
		//this will be converted to tuple of placeholders by granulate function afterward
		// default parameter T is a workaround for visual c++ 2013 CTP
		template <size_t M, size_t N, typename T = void>
		auto range(	const detail::placeholder_with_F<M, T, T>& ,
					const detail::placeholder_with_F<N, T, T>& ) ->detail::indEx_range<M, N+1>
		{
			return detail::indEx_range<M, N+1>{};
		}
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

//***********************************************************************************************

namespace std {
	template <size_t N, typename F, typename R>
		struct is_placeholder<my::detail::placeholder_with_F<N, R, F>>	{
			static constexpr size_t value = N;
		};
	//#ifdef BOOST_BIND_ARG_HPP_INCLUDED
	//template <> struct is_placeholder<boost::arg<1>> : integral_constant<int, 1> { };
	//...

}	//namespace std
