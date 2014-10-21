//functor_overload header
//Copyright (c) 2014 mmYYmmdd

#if !defined MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED
#define MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED

#include <type_traits>

namespace mymd {

	namespace detail_fol	{

		template <typename>
		struct noCondition	{
			static constexpr bool value = true;
		};

		template <template <typename> class C = noCondition, bool B = true>
		struct cond	{ };
		//-----------------------------------------------------
		template <typename...>
		struct types  { };
		//-----------------------------------------------------
		template <typename T, typename>
		struct arg  {
			using apply  = std::enable_if<true, T>;
			using applyr = apply;
		};
		
		template <template <typename> class C, bool B, typename V>
		struct arg<cond<C, B>, V>  {
			using apply  = std::enable_if<B == C<V>::value, V>;
			using applyr = std::enable_if<B == C<V>::value, V&&>;
		};
		//-----------------------------------------------------
		struct no_action	{
			constexpr no_action() { }
			template <typename... V>
			void operator ()(V&&...) const {}
		};
		//-----------------------------------------------------
		template <typename F, typename... A>
		class bolt  {
			F fn;
			template <typename T, typename U>
			using apply = typename arg<T, U>::apply;	//workaround for VC
		protected:
			template <typename... V>
			auto invoke(types<V...>, typename arg<A, V>::applyr::type... a) const
				->decltype(fn(std::declval<typename apply<A, V>::type>()...))
				{  return fn(std::forward<typename apply<A, V>::type>(a) ...);  }
			template <typename... V>
			auto invoke(types<V...>, typename arg<A, V>::applyr::type... a)
				->decltype(fn(std::declval<typename apply<A, V>::type>()...))
				{  return fn(std::forward<typename apply<A, V>::type>(a) ...);  }
		public:
			constexpr bolt(const F& f) : fn(f)  {  }
		};
		//-----------------------------------------------------
		template <typename T1, typename T2>
		class bolts : private T1, private T2  {
			protected: using T1::invoke; using T2::invoke;
		public:
			constexpr bolts(const T1& t1, const T2& t2) : T1(t1), T2(t2) { }
			template <typename... V>
			auto operator()(V&&... v) const->decltype(invoke(types<V...>{}, std::declval<V>()...))
				{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
			template <typename... V>
			auto operator()(V&&... v) ->decltype(invoke(types<V...>{}, std::declval<V>()...))
				{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
		};

		template <typename F, typename... A>
		class bolts<bolt<F, A...>, void> : private bolt<F, A...>  {
			protected : using bolt<F, A...>::invoke;
		public:
			constexpr bolts(const F& t) : bolt<F, A...>(t) { }
			template <typename... V>
			auto operator()(V&&... v) const->decltype(invoke(types<V...>{}, std::declval<V>()...))
				{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
			template <typename... V>
			auto operator()(V&&... v) ->decltype(invoke(types<V...>{}, std::declval<V>()...))
				{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
		};

		template <typename T1, typename T2, typename U1, typename U2>
		auto operator +(bolts<T1, T2> const& b, bolts<U1, U2> const& c) ->bolts<bolts<T1, T2>, bolts<U1, U2>>
		{
			return bolts<bolts<T1, T2>, bolts<U1, U2>>(b, c);
		}
		//-----------------------------------------------------
		template <typename F, typename... A>
		using gen_t = bolts<bolt<typename std::conditional<std::is_same<F, void>::value, no_action, F>::type, A...>, void>;

	}	//namespace detail_fol
	//********************************************************************
	//  user interfaces are ,
	//  mymd::gen<A, B, C...>(Fn)  ,  mymd::cond<P>  ,  operator +
	//********************************************************************
	using detail_fol::cond;		//  condition for type
	using detail_fol::bolts;		//  functor type
	using detail_fol::gen_t;		//  functor type generated by 'gen'

	template <typename... A, typename F>
	auto gen(const F& f) ->gen_t<F, A...>
	{	return gen_t<F, A...>(f);	}

	//no action
	template <typename... A>
	auto gen() ->gen_t<void, A...>
	{	return gen_t<void, A...>(detail_fol::no_action{});	}

}	//namespace mymd

#endif
