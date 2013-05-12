#ifndef _DATASIZE_H_
#define _DATASIZE_H_ 1

#include <stdlib.h>
#include "serialize.h"

#define MALLOC_OVERHEAD 8

size_t inline DataSize(const unsigned char &t)  { return sizeof(t); }
size_t inline DataSize(const char &t)           { return sizeof(t); }
size_t inline DataSize(const unsigned short &t) { return sizeof(t); }
size_t inline DataSize(const short &t)          { return sizeof(t); }
size_t inline DataSize(const unsigned int &t)   { return sizeof(t); }
size_t inline DataSize(const int &t)            { return sizeof(t); }
size_t inline DataSize(const unsigned long &t)  { return sizeof(t); }
size_t inline DataSize(const long &t)           { return sizeof(t); }
size_t inline DataSize(const int64 &t)          { return sizeof(t); }
size_t inline DataSize(const uint64 &t)         { return sizeof(t); }
size_t inline DataSize(const float &t)          { return sizeof(t); }
size_t inline DataSize(const double &t)         { return sizeof(t); }
size_t inline DataSize(const bool &t)           { return sizeof(t); }

template<typename T> size_t DataSize(const T *t) { return sizeof(t); }
template<typename T> size_t DataSize(T *t)       { return sizeof(t); }

template<typename C> size_t DataSize(const std::basic_string<C>& str);
template<typename T1, typename T2> size_t DataSize(const std::pair<T1,T2> &pair);
template<typename T, typename C, typename A> size_t DataSize(const std::set<T,C,A> &set);
template<typename K, typename T, typename C, typename A> size_t DataSize(const std::map<K,T,C,A> &map);
template<typename T> size_t DataSize(const T &t);

template<typename C> size_t DataSize(const std::basic_string<C>& str) {
    return 2*sizeof(size_t) + sizeof(int) + sizeof(C)*(str.length() + 1);
}

template<typename T, typename A> size_t DataSize(const std::vector<T, A> &v) { 
    size_t ret = 3*sizeof(void*) + MALLOC_OVERHEAD + (v.capacity() - v.size()) * sizeof(T);
    for (typename std::vector<T, A>::const_iterator it = v.begin(); it != v.end(); it++)
        ret += ::DataSize(*it);
    return ret;
}

template<typename T1, typename T2> size_t DataSize(const std::pair<T1,T2> &pair) { return ::DataSize(pair.first) + ::DataSize(pair.second); }

template<typename T, typename C, typename A> size_t DataSize(const std::set<T,C,A> &set) {
    size_t ret = sizeof(C) + 3*sizeof(void*) + sizeof(bool) + sizeof(size_t);
    for (typename std::set<T, C, A>::const_iterator it = set.begin(); it != set.end(); it++)
        ret += sizeof(bool) + 3*sizeof(void*) + ::DataSize(*it) + MALLOC_OVERHEAD;
    return ret;
}

template<typename K, typename T, typename C, typename A> size_t DataSize(const std::map<K,T,C,A> &map) {
    size_t ret = sizeof(C) + 3*sizeof(void*) + sizeof(bool) + sizeof(size_t);
    for (typename std::map<K, T, C, A>::const_iterator it = map.begin(); it != map.end(); it++)
        ret += sizeof(bool) + 3*sizeof(void*) + ::DataSize(it->first) + ::DataSize(it->second) + MALLOC_OVERHEAD;
    return ret;
}

template<typename T> size_t DataSize(const T &t) { return t.GetDataSize(); }

#endif
