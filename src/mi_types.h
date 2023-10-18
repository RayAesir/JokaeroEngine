#pragma once

// deps
#include <mimalloc.h>
// global
#include <unordered_map>
#include <vector>

template <typename T>
using MiVector = std::vector<T, mi_stl_allocator<T>>;

template <typename K, typename T>
using MiUnMap = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>,
                                   mi_stl_allocator<std::pair<const K, T>>>;
