#pragma once
#include <type_traits>

template<typename FunctionType>
struct FunctionTraits;

template<typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType(Args...)>
{
    using return_type = ReturnType;
};

