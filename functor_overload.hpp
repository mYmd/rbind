//functor_overload header
//Copyright (c) 2014 mmYYmmdd

#if !defined MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED
#define MMYYMMDD_FUNCTOR_OVERLOAD_INCLUDED

#include <type_traits>

namespace mymd {
	namespace detail_fo	{

		template <typename>
		struct noCondition	{
			static constexpr bool value = true;
		};

		template <template <typename> class C = noCondition, bool B = true>
		struct cond	{ };
		//-----------------------------------------------------
		template <typename...>
		struct types  { };
		
		template <typename T, typename>
		struct arg  {
			using apply = std::enable_if<true, T>;
		};
		
		template <template <typename> class C, bool B, typename V>
		struct arg<cond<C, B>, V>  {
			using apply = std::enable_if<B == C<V>::value, V>;
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
			template <typename T, typename U>	//workaround for VC
			using apply = typename arg<T, U>::apply;
		protected:
			template <typename... V>
			auto invoke(types<V...>, typename arg<A, V>::apply::type&&... a) const
				->decltype(fn(std::declval<typename apply<A, V>::type>()...))
			{  return fn(std::forward<typename apply<A, V>::type>(a) ...);  }
		public:
			constexpr bolt(const F& f) : fn(f)  {  }
		};

		template <typename T1, typename T2>
		struct bolts : private T1, private T2  {
            using T1::operator ();
            using T2::operator ();
			constexpr bolts(const T1& t1, const T2& t2) : T1(t1), T2(t2) { }
		};

		template <typename F, typename... A>
		struct bolts<bolt<F, A...>, void> : private bolt<F, A...>  {
            protected: using bolt<F, A...>::invoke;
        public:
			constexpr bolts(const bolt<F, A...>& t) : bolt<F, A...>(t) { }
			template <typename... V>
			auto operator()(V&&... v) const->decltype(invoke(types<V...>{}, std::declval<V>()...))
			{  return invoke(types<V...>{}, std::forward<V>(v)...);  }
		};

		template <typename T1, typename T2, typename U1, typename U2>
		auto operator +(bolts<T1, T2> const& b, bolts<U1, U2> const& c)
			->bolts<bolts<T1, T2>, bolts<U1, U2>>
		{
			return bolts<bolts<T1, T2>, bolts<U1, U2>>(b, c);
		}

	}	//namespace detail_fo

	//user interfaces are ,
	//  mymd::gen< >( )  ,  mymd::cond< >  ,  operator +

	using detail_fo::cond;

	template <typename... A, typename F>
	auto gen(const F& f) ->detail_fo::bolts<detail_fo::bolt<F, A...>, void>
	{
		return detail_fo::bolts<detail_fo::bolt<F, A...>, void>(detail_fo::bolt<F, A...>(f));
	}

	//no action
	template <typename... A>
	auto gen() ->detail_fo::bolts<detail_fo::bolt<detail_fo::no_action, A...>, void>
	{
		return detail_fo::bolts<detail_fo::bolt<detail_fo::no_action, A...>, void>
			(detail_fo::bolt<detail_fo::no_action, A...>(detail_fo::no_action{}));
	}

	//----------------------------------
	//template <typename T>
	//using is_xxx = mymd::is_some_rr<std::is_yyyy, std::is_zzzz>::apply<T>;
	//using is_xxx = mymd::is_every_rr<std::is_yyyy, std::is_zzzz>::apply<T>;

	template <template <typename> class first, template <typename> class... tail>
	struct is_some_rr	{
		template <typename T>
		struct apply	{
			using T2 = typename std::remove_reference<T>::type;
			static constexpr bool value = first<T2>::value || is_some_rr<tail...>::template apply<T>::value;
		};
	};

	template <template <typename> class first>
	struct is_some_rr<first>	{
		template <typename T>
		struct apply	{
			using T2 = typename std::remove_reference<T>::type;
			static constexpr bool value = first<T2>::value;
		};
	};

	template <template <typename> class first, template <typename> class... tail>
	struct is_every_rr	{
		template <typename T>
		struct apply	{
			using T2 = typename std::remove_reference<T>::type;
			static constexpr bool value = first<T2>::value && is_every_rr<tail...>::template apply<T>::value;
		};
	};

	template <template <typename> class first>
	struct is_every_rr<first>	{
		template <typename T>
		struct apply	{
			using T2 = typename std::remove_reference<T>::type;
			static constexpr bool value = first<T2>::value;
		};
	};
}

#endif
