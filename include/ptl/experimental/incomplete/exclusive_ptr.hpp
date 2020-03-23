#pragma once

template<typename T, typename FN>
struct TypedDeleter;

template<typename T, typename ReturnType, typename... Args>
struct TypedDeleter
{

};

template<typename T, typename DELETER>
struct exclusive_ptr {

};