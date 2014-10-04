//functor_overload header
//Copyright (c) 2014 mmYYmmdd

#if !defined MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED
#define MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED

#include <type_traits>

namespace mymd	{
	namespace detail_fo	{
		template <template <typename> class C>	struct cond  { };

		template <typename...>	struct types  { };
		
		template <typename T, typename>
		struct arg  {
			using apply = std::enable_if<true, T>;
		};
		
		template <template <typename> class C, typename V>
		struct arg<cond<C>, V>  {
			using apply = std::enable_if<C<V>::value, V>;
		};

		template <typename F, typename... A>
		class bolt  {
			F fn;
		protected:
			template <typename... V>
			auto invoke(types<V...>, typename arg<A, V>::apply::type... a) const
				->decltype(fn(std::declval<typename arg<A, V>::apply::type>()...))
			{  return fn(std::forward<typename arg<A, V>::apply::type>(a)...);  }
		public:
			constexpr bolt(const F& f) : fn(f)  {  }
		};

		template <typename T1, typename T2>
		struct bolts : T1, T2  {
			constexpr bolts(const T1& t1, const T2& t2) : T1(t1), T2(t2) { }
			using T1::invoke;
			using T2::invoke;
			template <typename... V>
			auto operator()(V... v) const->decltype(invoke(types<V...>{}, std::declval<V>()...))
			{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
		};

		template <typename F, typename... A>
		struct bolts<bolt<F, A...>, void> : bolt<F, A...>  {
			constexpr bolts(const bolt<F, A...>& t) : bolt<F, A...>(t) { }
			template <typename... V>
			auto operator()(V... v) const->decltype(invoke(types<V...>{}, std::declval<V>()...))
			{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
		};

		template <typename T1, typename T2, typename U1, typename U2>
		auto operator +(bolts<T1, T2> const& b, bolts<U1, U2> const& c)
			->bolts<bolts<T1, T2>, bolts<U1, U2>>
		{
			return bolts<bolts<T1, T2>, bolts<U1, U2>>(b, c);
		}
	}	//namespace detail_fo

	//user interfases are ,
	//  mymd::gen< >() and mymd::cond< > and operator +

	using detail_fo::cond;

	template <typename... A, typename F>
	auto gen(const F& f) ->detail_fo::bolts<detail_fo::bolt<F, A...>, void>
	{
		return detail_fo::bolts<detail_fo::bolt<F, A...>, void>(detail_fo::bolt<F, A...>(f));
	}
}

#endif
