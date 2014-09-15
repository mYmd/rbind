//rbindv header
//Copyright (c) 2014 mmYYmmdd

#if !defined MMYYMMDD_RBIND_AND_PLACEHOLDERS_INCLUDED
#define MMYYMMDD_RBIND_AND_PLACEHOLDERS_INCLUDED

#include <functional>

namespace mymd	{
namespace detail	{

	struct nil	{	};

	//============================================================================
	template <std::size_t... indices>
	struct indEx_sequence	{
		static constexpr std::size_t size()	{	return sizeof...(indices);	}
	};

	// linear recursion is sufficient for bind parameters   線形再帰で十分
	template <std::size_t first, std::size_t last, typename result = indEx_sequence<>, bool flag = (first >= last)>
	struct indEx_range_imple		{
		typedef result type;
	};

	template <std::size_t first, std::size_t last, std::size_t... indices>
	struct	indEx_range_imple<first, last, indEx_sequence<indices...>, false> :
					indEx_range_imple<first + 1, last, indEx_sequence<indices..., first>>		{
	};

	template <std::size_t first, std::size_t last>
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

	template<typename T>
	using remove_ref_cv_t = typename remove_ref_cv<T>::type;

	// type compair without const/volatile and reference   cv属性と参照属性を外して型の同一性を比較
	template<typename T, typename U>
		struct is_same_ignoring_cv_ref :
			std::is_same<remove_ref_cv_t<T>, remove_ref_cv_t<U>>	{	};

	// remove_rvalue_reference
	template<class T>
	struct remOve_rvaluE_reference	{
		typedef T type;
	};

	template<class T>
	struct remOve_rvaluE_reference<T&&>	{
		typedef T type;
	};

	template<class T>
	using remOve_rvaluE_reference_t = typename remOve_rvaluE_reference<T>::type;

	//==================================================================

	//parameter buffer    パラメータバッファ
	template <typename T>
	struct param_buf	{
		T	val;		//std::unique_ptr<T>
		typedef T	type;
		param_buf(T&& t) : val(std::forward<T>(t))	{ }
		T& get()	{	return val;	}
	};
	//		ref
	template <typename T>
	struct param_buf<T&>	{
		T&	val;
		typedef T&	type;
		param_buf(T& t) : val(t)	{ }
		T& get()	{ return val; }
	};

	//************************************************************************************************
	//parameter adaptation judge by Tr<V>::value
	template <template <typename>class Tr>
	struct condition_trait_1	{};

	//parameter adaptation judge by Tr::template apply<V>::value
	template <typename Tr>
	struct condition_trait_2	{};
	//************************************************************************************************

	//placeholder =======================================================================
	//with default parameter    デフォルト引数を付与されたplaceholder
	template <std::size_t N, typename T>
	struct placeholder_with_V	{
		mutable param_buf<T> elem;
		placeholder_with_V(T&& t) : elem(std::forward<T>(t))	{	}
		T& get() const	{	return elem.get();	}
	};

	//parameter type adaptation 1
	template <std::size_t N, template <typename>class Tr>
	struct placeholder_with_V<N, condition_trait_1<Tr>>	{};

	//parameter type adaptation 2
	template <std::size_t N, typename Tr>
	struct placeholder_with_V<N, condition_trait_2<Tr>>	{};

	//basic placeholder    基本のplaceholder
	template <std::size_t N>
	struct placeholder_with_V<N, void>	{
		constexpr placeholder_with_V()	{	}
		//assign the default parameter    デフォルト値を = で設定する
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

	template <std::size_t N>
	using simple_placeholder = placeholder_with_V<N, void>;

	//=======================================================================================
	//convet from placeholder to argument    placeholderから実引数に変換 ====================
	//not my placeholder    mymd::detail::placeholder以外
	template <typename T>
	struct parameter_evaluate	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(T& , V&& v)
				{	return std::forward<V>(v);	}
			};
	};

	//with default value    デフォルト値を与えられた
	template <std::size_t N, typename T>
	struct parameter_evaluate<placeholder_with_V<N, T>>	{
		template <typename V, typename W = remove_ref_cv_t<V>>
			struct eval	{			//if argument assigned    実引数あり
				typedef V	type;
				static V&& get(placeholder_with_V<N, T> const& , V&& v)
				{	return std::forward<V>(v);	}
			};
		template <typename V>
			struct eval<V, nil>	{	//without argument    実引数なし → デフォルト値
				typedef T&	type;
				static T& get(placeholder_with_V<N, T> const& t, V&& )
				{	return t.get();	}
			};
	};

	//assert parameter type 2
	template <std::size_t N, template <typename>class Tr>
	struct parameter_evaluate<placeholder_with_V<N, condition_trait_1<Tr>>>	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(placeholder_with_V<N, condition_trait_1<Tr>> const& , V&& v)
				{
					static_assert(Tr<remOve_rvaluE_reference_t<V>>::value, "parameter type is rejected");
					return std::forward<V>(v);
				}
			};
	};

	//assert parameter type 2
	template <std::size_t N, typename Tr>
	struct parameter_evaluate<placeholder_with_V<N, condition_trait_2<Tr>>>	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(placeholder_with_V<N, condition_trait_2<Tr>> const& , V&& v)
				{
					static_assert(Tr::template apply<remOve_rvaluE_reference_t<V>>::value, "parameter type is rejected");
					return std::forward<V>(v);
				}
			};
	};

	//basic    基本
	template <std::size_t N>
	struct parameter_evaluate<simple_placeholder<N>>	{
		template <typename V>
			struct eval	{
				typedef V	type;
				static V&& get(simple_placeholder<N> const& , V&& v)
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
	template <typename C, typename R, std::size_t N>
	struct sig_type		{
		typedef C	call_type;
		typedef R	result_type;
		static constexpr std::size_t value = N;
	};
	//---------------------------------------------------------------------------------
	//	[type0, type1, type2, ..., typeM, nil) ... typeN ,,,
	//       0,     1,     2, ....        M+1
	template<typename... Tail>
	struct nil_stop	{
		static constexpr std::size_t value = 0;
	};

	template<typename Head, typename... Tail>
	struct nil_stop<Head, Tail...>	{
		static constexpr std::size_t value = is_same_ignoring_cv_ref<Head, nil>::value?
												0								:
												1 + nil_stop<Tail...>::value	;
	};

	template<typename... Params>
	struct nil_stop<std::tuple<Params...>>	{
		static constexpr std::size_t value = nil_stop<Params...>::value;
	};

	//--------------------------------------------------------------------
	//handle a type as a raw ponter    スマートポインタ等を生ポインタと同様に扱うための変換 =======
	// int => nil*,  int* => int*,  smart_ptr<int> => int*,  itr<int> => int*
	template <typename T>
	class raw_ptr_type	{
		template <typename U>
			static std::true_type test1(U&& u, decltype(*u, static_cast<void>(0), 0) = 0);
		static std::false_type	test1(...);
		static constexpr bool flag = decltype(test1(std::declval<T>()))::value;
		using psp_type = typename std::conditional<flag , T, nil*>::type; 
	public:
		typedef decltype(&*std::declval<psp_type>())	type;
	};

	//--------------------------------------------------------------------
	//determin the type of invocation
	template<typename TPL>
	class invokeType	{		//	SFINAE
		template<std::size_t N>
			static auto get()->decltype(std::declval<typename std::tuple_element<N, TPL>::type>());
		static constexpr std::size_t ParamSize = nil_stop<TPL>::value;
		//
		template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_sequence<indices1...>	,
							indEx_sequence<indices2...>	,
							T0&&					t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T0>(t0)(get<indices1>()...), static_cast<void>(0), 1) = 0	)
			->sig_type<	fnc_c	,
						decltype(std::forward<T0>(t0)(get<indices1>()...))	,
						ParamSize - 1	>;
		//
		template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_sequence<indices1...>	,
							indEx_sequence<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1>(t1).*t0)(get<indices2>()...), static_cast<void>(0), 1) = 0	)
			->sig_type<	memF_c	,
						decltype((std::forward<T1>(t1).*t0)(get<indices2>()...))	,
						ParamSize - 2	>;
		//
		template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_sequence<indices1...>	,
							indEx_sequence<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype((std::forward<T1P>(t1p)->*t0)(get<indices2>()...), static_cast<void>(0), 1) = 0	)
			->sig_type<	memF_c*		,
						decltype((std::forward<T1P>(t1p)->*t0)(get<indices2>()...))	,
						ParamSize - 2	>;
		//
		template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_sequence<indices1...>	,
							indEx_sequence<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1>(t1).*t0, static_cast<void>(0), 1) = 0	)
			->sig_type<	mem_c	,
						decltype(std::forward<T1>(t1).*t0)	,
						ParamSize - 2	>;
		//
		template<std::size_t... indices1, std::size_t... indices2, typename T0, typename T1, typename T1P>
		static auto test(	indEx_sequence<indices1...>	,
							indEx_sequence<indices2...>	,
							T0						t0	,
							T1&&					t1	,
							T1P&&					t1p	,
							decltype(std::forward<T1P>(t1p)->*t0, static_cast<void>(0), 1) = 0	)
			->sig_type<	mem_c*	,
						decltype(std::forward<T1P>(t1p)->*t0)	,
						ParamSize - 2	>;
		//
		using raw_p_t = typename raw_ptr_type<remove_ref_cv_t<typename std::tuple_element<1, TPL>::type>>::type;
		//
		using sig_t = decltype(test(indEx_range<1, ParamSize>{}	,
									indEx_range<2, ParamSize>{}	,
									get<0>()					,
									get<1>()					,
									std::declval<raw_p_t>()		));
	public:
		typedef typename sig_t::call_type			call_type;
		typedef typename sig_t::result_type			result_type;
		typedef indEx_range<0, sig_t::value>		actual_indice;
	};

	//************************************************************************************************
	// execute
	template <typename R, typename T, typename A> struct executor;

	//member
	template <typename R, std::size_t... indices>
	struct executor<R, mem_c, indEx_sequence<indices...>>		{
		template <typename M, typename Obj>
		R exec(M mem, Obj&& obj) const
		{	return (std::forward<Obj>(obj)).*mem;	}
	};
	//----
	template <typename R, std::size_t... indices>
	struct executor<R, mem_c*, indEx_sequence<indices...>>		{
		template <typename M, typename pObj>
		R exec(M mem, pObj&& pobj) const
		{	return (*std::forward<pObj>(pobj)).*mem;	}
	};

	//member function
	template <typename R, std::size_t... indices>
	struct executor<R, memF_c, indEx_sequence<indices...>>	{
		template <typename M, typename Obj, typename... Params>
		R exec(M mem, Obj&& obj, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return ((std::forward<Obj>(obj)).*mem)(std::get<indices>(vt)...);
		}
	};
	//----
	template <typename R, std::size_t... indices>
	struct executor<R, memF_c*, indEx_sequence<indices...>>	{
		template <typename M, typename pObj, typename... Params>
		R exec(M mem, pObj&& pobj, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return ((*std::forward<pObj>(pobj)).*mem)(std::get<indices>(vt)...);
		}
	};

	//functor
	template <typename R, std::size_t... indices>
	struct executor<R, fnc_c, indEx_sequence<indices...>>	{
		template <typename F, typename... Params>
		R exec(F&& f, Params&&... params) const
		{
			auto vt = std::forward_as_tuple(std::forward<Params>(params)...);
			return (std::forward<F>(f))(std::get<indices>(vt)...);
		}
	};

	//************************************************************************************************
	//nested rbind to value at the same time  同時に評価するネストされたrbind
	template <typename... Vars>
	struct BindOfNested;
    
	template <typename>
	struct is_nested   {
        static constexpr bool value = false;
    };

    template <typename... Vars>
	struct is_nested<BindOfNested<Vars...>>   {
        static constexpr bool value = true;
    };

	//----------------------------------------------------------------------------
	//a parameter    パラメータ
	template <typename P>
	struct ParamOf	{
		mutable param_buf<P> elem;
		using p_h  = remove_ref_cv_t<P>;
		typedef P	param_t;
		static constexpr std::size_t placeholder = std::is_placeholder<p_h>::value;
        static constexpr bool nested = is_nested<p_h>::value;
		ParamOf(P&& p) : elem(std::forward<P>(p))	{	}
		P& get() const	{	return elem.get();	}
	};
	//----------------------------------------------------------------------------
	template <typename P0, typename Params, std::size_t N, bool B, bool nest>
	class alt_param	{
		using ph     = remove_ref_cv_t<typename P0::param_t>;
		using type_a = typename std::tuple_element<N-1, Params>::type;
		using vtype  = typename parameter_evaluate<ph>::template eval<type_a>;
	public:
		typedef typename vtype::type			type;
		static type get(const P0& p, Params&& params)
		{
			return vtype::get(p.get(), std::get<N-1>(std::forward<Params>(params)));
		}
	};
	//not placeholder
	template <typename P0, typename Params>
	struct alt_param<P0, Params, 0, true, false>	{
		typedef typename P0::param_t&			type;	//ここ
		static type get(const P0& p, Params&& )
		{
			return p.get();
		}
	};
	//out of range
	template <typename P0, typename Params, std::size_t N>
	class alt_param<P0, Params, N, false, false>		{		//N is out of range
		using ph    = remove_ref_cv_t<typename P0::param_t>;
		using vtype = typename parameter_evaluate<ph>::template eval<nil>;
	public:
		typedef typename vtype::type			type;
		static type get(const P0& p, Params&& )
		{
			return vtype::get(p.get(), nil{});
		}
	};
	//nested rbind to value at the same time  同時に評価するネストされたrbind
	template <typename P0, typename Params>
	class alt_param<P0, Params, 0, true, true>		{		//
        using nested = remove_ref_cv_t<typename P0::param_t>;
        static constexpr std::size_t N = nested::N;
	public:
		typedef decltype(std::declval<nested>().call_imple(std::declval<Params>(), indEx_range<0, N>{}))	type;
		static type get(const P0& p, Params&& params)
		{
			return p.get().call_imple(std::forward<Params>(params), indEx_range<0, N>{});
		}
	};//*/
	//----------------------------------------------------------------------------
	template <std::size_t N, typename Params1, typename Params2>
	class param_result	{
		using P0 = typename std::tuple_element<N, Params1>::type;
		static constexpr std::size_t	placeholder	= P0::placeholder;
		static constexpr bool small = (placeholder <= std::tuple_size<Params2>::value);
		using alt_t = alt_param<P0, Params2, placeholder, small, P0::nested>;
	public:
		typedef typename alt_t::type		result_type;
		static result_type get(const Params1& params1, Params2&& params2)
		{
			return alt_t::get(std::get<N>(params1), std::forward<Params2>(params2));
		}
	};

	template<std::size_t N, typename Params1, typename Params2>
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
		template<typename Params2T, std::size_t... indices>
		struct invoke_type_i<Params2T, indEx_sequence<indices...>>	{
			static const Params1&	get1();
			using ParamTuple = decltype(std::forward_as_tuple(
									mymd::detail::template param_at<indices>(get1(), std::declval<Params2T>())...)
										);
			using invoke_type   = typename mymd::detail::invokeType<ParamTuple>;
			using actual_indice = typename invoke_type::actual_indice;
			using call_type     = typename invoke_type::call_type;
			using result_type   = typename invoke_type::result_type;
			using Executor_t    = executor<result_type, call_type, actual_indice>;
		};
		//
	protected:
		template<typename Params2T, std::size_t... indices>
		auto call_imple(Params2T&& params2, indEx_sequence<indices...> ) const
			->typename invoke_type_i<Params2T, indEx_sequence<indices...>>::result_type
		{
			typename invoke_type_i<Params2T, indEx_sequence<indices...>>::Executor_t	Executor;
			return 
				Executor.exec(
					param_at<indices>(params1, std::forward<Params2T>(params2))...
				);
		}
	public:
		template <std::size_t... indices>
		BindOf(std::tuple<Vars...>&& vars, indEx_sequence<indices...>) :
				params1(std::get<indices>(std::forward<std::tuple<Vars...>>(vars))...)	{	}
		//
		static constexpr std::size_t	N =  sizeof...(Vars);
		//
		template <typename... Params2>
		auto operator ()(Params2&&... params2) const
			->typename invoke_type_i<std::tuple<Params2...>, indEx_range<0, N>>::result_type
		{
			return call_imple(	std::forward_as_tuple(std::forward<Params2>(params2)...),
								indEx_range<0, N>{}
							);
		}
		//
		auto operator *() const ->BindOfNested<Vars...>;
	};

	//BindOf is not a pointer type       BindOfはポインタじゃない
	template <typename... Vars>
	class raw_ptr_type<BindOf<Vars...>>	{
	public:
		typedef nil* type;
	};

	//*******************************************************************
	//nested rbind to value at the same time  同時に評価するネストされたrbind
	template <typename... Vars>
	struct BindOfNested : private BindOf<Vars...>	{
		BindOfNested(const BindOf<Vars...>& b) : BindOf<Vars...>(b)			{ }
		BindOfNested(BindOf<Vars...>&& b) : BindOf<Vars...>(std::move(b))	{ }
		using BindOf<Vars...>::call_imple;
        static constexpr std::size_t	N =  sizeof...(Vars);
	};

	template <typename... Vars>
	auto BindOf<Vars...>::operator *() const ->BindOfNested<Vars...>
	{
		return BindOfNested<Vars...>(*this);
	}

	//*******************************************************************
	template <typename T>
	auto granulate(T&& t)->std::tuple<T&&>
	{
		return std::forward_as_tuple(std::forward<T>(t));
	}

	template <std::size_t... indices>
	auto granulate(indEx_sequence<indices...>&& )
		-> std::tuple<placeholder_with_V<indices, void>...>
	{
		return std::tuple<placeholder_with_V<indices, void>...>();
	};

	template <typename... T>
	auto untie_vars(T&&... t)->decltype(std::tuple_cat(granulate(std::forward<T>(t))...))
	{
		return std::tuple_cat(granulate(std::forward<T>(t))...);
	}
	//-------------------------------------------------
	template <typename... Vars>
	auto rbind_imple(std::tuple<Vars...>&& vars)->BindOf<remOve_rvaluE_reference_t<Vars>...>
	{
		using index = indEx_range<0, sizeof...(Vars)>;
		return BindOf<remOve_rvaluE_reference_t<Vars>...>(std::forward<std::tuple<Vars...>>(vars), index());
	}
	//-------------------------------------------------
	template <typename T>
	struct emplace_t	{
		template <typename... Vars>
		T operator()(Vars&&... vars) const
		{	return T(std::forward<Vars>(vars)...);	}
	};

} //namespace mymd::detail

	//predefined placeholders _1st, _2nd, _3rd, _4th, ...      定義済みプレースホルダ 
	namespace placeholders	{
		namespace {
			constexpr detail::simple_placeholder< 1>  _1st;
			constexpr detail::simple_placeholder< 2>  _2nd;
			constexpr detail::simple_placeholder< 3>  _3rd;
			constexpr detail::simple_placeholder< 4>  _4th;
			constexpr detail::simple_placeholder< 5>  _5th;
			constexpr detail::simple_placeholder< 6>  _6th;
			constexpr detail::simple_placeholder< 7>  _7th;
			constexpr detail::simple_placeholder< 8>  _8th;
			constexpr detail::simple_placeholder< 9>  _9th;
			constexpr detail::simple_placeholder<10> _10th;
			constexpr detail::simple_placeholder<11> _11th;
			constexpr detail::simple_placeholder<12> _12th;
			constexpr detail::simple_placeholder<13> _13th;
			constexpr detail::simple_placeholder<14> _14th;
			constexpr detail::simple_placeholder<15> _15th;
			//   ...
		}

		// Alias for placeholder
		template <std::size_t N>
		using plhdr_t = detail::simple_placeholder<N>;

		//until(_9th) => [_1st, _2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
		//this will be converted to tuple of placeholders by granulate function afterward
		// default parameter T is a workaround for visual c++ 2013 CTP
		template <std::size_t N, typename T = void>
		auto until(const detail::placeholder_with_V<N, T>& ) ->detail::indEx_range<1, N+1>
		{
			return detail::indEx_range<1, N+1>{};
		}

		//range(_2nd, _9th) => [_2nd, _3rd, _4th, _5th, _6th, _7th, _8th, _9th]
		//this will be converted to tuple of placeholders by granulate function afterward
		// default parameter T is a workaround for visual c++ 2013 CTP
		template <std::size_t M, std::size_t N, typename T = void>
		auto range(	const detail::placeholder_with_V<M, T>& ,
					const detail::placeholder_with_V<N, T>& ) ->detail::indEx_range<M, N+1>
		{
			return detail::indEx_range<M, N+1>{};
		}
	}	// mymd::placeholders

	//************************************************************************
	//user interface / rbind functions       ユーザが使う rbind関数
	template <typename... Vars>
	auto rbind(Vars&&... vars)
		->decltype(detail::rbind_imple(detail::untie_vars(std::forward<Vars>(vars)...)))
	{
		return detail::rbind_imple(detail::untie_vars(std::forward<Vars>(vars)...));
	}
	//construct of typeT   型Tのコンストラクタ呼び出し
	template <typename T, typename... Vars>
	auto emplace(Vars&&... vars) ->decltype(rbind(detail::emplace_t<T>{}, std::forward<Vars>(vars)...))
	{
		return rbind(detail::emplace_t<T>{}, std::forward<Vars>(vars)...);
	}

} //namespace mymd

//***********************************************************************************************

namespace std {
	template <std::size_t N, typename T>
		struct is_placeholder<mymd::detail::placeholder_with_V<N, T>>	{
			static constexpr std::size_t value = N;
		};
	//#ifdef BOOST_BIND_ARG_HPP_INCLUDED
	//template <> struct is_placeholder<boost::arg<1>> : integral_constant<int, 1> { };
	//...

}	//namespace std

#endif
