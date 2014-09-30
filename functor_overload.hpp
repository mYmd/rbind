//functor_overload header
//Copyright (c) 2014 mmYYmmdd

#if !defined MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED
#define MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED

namespace mymd	{

	namespace detail	{

		template <typename F, typename... A>
		class bolt  {
			F fn;
		public:
			constexpr bolt(const F& f) : fn(f)  {  }
			auto operator()(A... a) const->decltype(fn(std::forward<A>(a)...))
			{  return fn(std::forward<A>(a)...);  }
		};

		template <typename T1, typename T2>
		struct bolts : T1, T2  {
			constexpr bolts(const T1& t1, const T2& t2) : T1(t1), T2(t2) { }
			using T1::operator ();
			using T2::operator ();
		};
    
		template <typename F, typename... A>
		struct bolts<bolt<F, A...>, void> : bolt<F, A...>  {
			constexpr bolts(const bolt<F, A...>& t) : bolt<F, A...>(t) { }
			//using bolt<F, A...>::operator ();
		};

		template <typename T1, typename T2, typename U1, typename U2>
		auto
		operator +(bolts<T1, T2> const& b, bolts<U1, U2> const& c)
			->bolts<bolts<T1, T2>, bolts<U1, U2>>
		{
			return bolts<bolts<T1, T2>, bolts<U1, U2>>(b, c);
		}

	}

	template <typename... A, typename F>
	auto gen(const F& f) ->detail::bolts<detail::bolt<F, A...>, void>
	{
		return detail::bolts<detail::bolt<F, A...>, void>(detail::bolt<F, A...>(f));
	}

}

#endif
