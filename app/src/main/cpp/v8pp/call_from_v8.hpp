//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_CALL_FROM_V8_HPP_INCLUDED
#define V8PP_CALL_FROM_V8_HPP_INCLUDED

#include <functional>

#include <v8.h>

#include "v8pp/convert.hpp"
#include "v8pp/utility.hpp"

namespace v8pp { namespace detail {

template<typename F, size_t Offset = 0>
struct call_from_v8_traits
{
	static bool const is_mem_fun = std::is_member_function_pointer<F>::value;
	using arguments = typename function_traits<F>::arguments;

	static size_t const arg_count =
		std::tuple_size<arguments>::value - is_mem_fun - Offset;

	template<size_t Index, bool>
	struct tuple_element
	{
		using type = typename std::tuple_element<Index, arguments>::type;
	};

	template<size_t Index>
	struct tuple_element<Index, false>
	{
		using type = void;
	};

	template<size_t Index>
	using arg_type = typename tuple_element<Index + is_mem_fun,
		Index < (arg_count + Offset)>::type;

	template<size_t Index, typename Traits, typename Arg = arg_type<Index>,
		typename T = typename std::remove_reference<Arg>::type,
		typename U = typename std::remove_pointer<T>::type
	>
	using arg_convert = typename std::conditional<
		is_wrapped_class<U>::value,
		typename std::conditional<
			std::is_pointer<T>::value,
			typename Traits::template convert_ptr<U>,
			typename Traits::template convert_ref<U>
		>::type,
		convert<Arg>
	>::type;

	template<size_t Index, typename Traits>
	static decltype(arg_convert<Index, Traits>::from_v8(std::declval<v8::Isolate*>(), std::declval<v8::Local<v8::Value>>()))
	arg_from_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		return arg_convert<Index, Traits>::from_v8(args.GetIsolate(), args[Index - Offset]);
	}

	static void check(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (args.Length() != arg_count)
		{
			throw std::runtime_error("argument count does not match function definition");
		}
	}
};

template<typename F>
using isolate_arg_call_traits = call_from_v8_traits<F, 1>;

template<typename F, size_t Offset = 0>
struct v8_args_call_traits : call_from_v8_traits<F, Offset>
{
	template<size_t Index, typename Traits>
	static v8::FunctionCallbackInfo<v8::Value> const&
	arg_from_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		return args;
	}

	static void check(v8::FunctionCallbackInfo<v8::Value> const&)
	{
	}
};

template<typename F>
using isolate_v8_args_call_traits = v8_args_call_traits<F, 1>;

template<typename F, size_t Offset>
using is_direct_args = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == (Offset + 1) &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<Offset>,
		v8::FunctionCallbackInfo<v8::Value> const&>::value>;

template<typename F>
using is_first_arg_isolate = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count != 0 &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>,
		v8::Isolate*>::value>;

template<typename F>
using select_call_traits = typename std::conditional<is_first_arg_isolate<F>::value,
	typename std::conditional<is_direct_args<F, 1>::value,
		isolate_v8_args_call_traits<F>,
		isolate_arg_call_traits<F>>::type,
	typename std::conditional<is_direct_args<F, 0>::value,
		v8_args_call_traits<F>,
		call_from_v8_traits<F>>::type
>::type;

template<typename Traits, typename F, typename CallTraits, size_t ...Indices>
typename function_traits<F>::return_type
call_from_v8_impl(F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	CallTraits, index_sequence<Indices...>)
{
	return func(CallTraits::template arg_from_v8<Indices, Traits>(args)...);
}

template<typename Traits, typename T, typename F, typename CallTraits, size_t ...Indices>
typename function_traits<F>::return_type
call_from_v8_impl(T& obj, F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	CallTraits, index_sequence<Indices...>)
{
	return (obj.*func)(CallTraits::template arg_from_v8<Indices, Traits>(args)...);
}

template<typename Traits, typename F, size_t ...Indices>
typename function_traits<F>::return_type
call_from_v8_impl(F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	isolate_arg_call_traits<F>, index_sequence<Indices...>)
{
	return func(args.GetIsolate(),
		isolate_arg_call_traits<F>::template arg_from_v8<Indices + 1, Traits>(args)...);
}

template<typename Traits, typename T, typename F, size_t ...Indices>
typename function_traits<F>::return_type
call_from_v8_impl(T& obj, F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	isolate_arg_call_traits<F>, index_sequence<Indices...>)
{
	return (obj.*func)(args.GetIsolate(),
		isolate_arg_call_traits<F>::template arg_from_v8<Indices + 1, Traits>(args)...);
}

template<typename Traits, typename F, size_t ...Indices>
typename function_traits<F>::return_type
call_from_v8_impl(F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	isolate_v8_args_call_traits<F>, index_sequence<Indices...>)
{
	return func(args.GetIsolate(), args);
}

template<typename Traits, typename T, typename F, size_t ...Indices>
typename function_traits<F>::return_type
call_from_v8_impl(T& obj, F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	isolate_v8_args_call_traits<F>, index_sequence<Indices...>)
{
	return (obj.*func)(args.GetIsolate(), args);
}

template<typename Traits, typename F>
typename function_traits<F>::return_type
call_from_v8(F&& func, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	using call_traits = select_call_traits<F>;
	call_traits::check(args);
	return call_from_v8_impl<Traits>(std::forward<F>(func), args,
		call_traits(), make_index_sequence<call_traits::arg_count>());
}

template<typename Traits, typename T, typename F>
typename function_traits<F>::return_type
call_from_v8(T& obj, F&& func, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	using call_traits = select_call_traits<F>;
	call_traits::check(args);
	return call_from_v8_impl<Traits>(obj, std::forward<F>(func), args,
		call_traits(), make_index_sequence<call_traits::arg_count>());
}

}} // v8pp::detail

#endif // V8PP_CALL_FROM_V8_HPP_INCLUDED
