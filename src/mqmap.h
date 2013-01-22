#ifndef _MQMAP_H_
#define _MQMAP_H_ 1

#include <map>

template<typename K, typename V> class _mqmap_iterator;
template<typename K, typename V> struct _mqmap_entry_type {
    V _mqmap_value;
    _mqmap_iterator<K,V> _mqmap_next;
    _mqmap_iterator<K,V> _mqmap_prev;
    int _mqmap_queue;

    _mqmap_entry_type<K,V>(_mqmap_iterator<K,V> none) : _mqmap_next(none), _mqmap_prev(none), _mqmap_queue(-1) {}
};

template<typename K, typename V> struct _mqmap_types {
    typedef _mqmap_entry_type<K,V> backend_entry_type;
    typedef std::map<K, backend_entry_type> backend_type;
    typedef typename backend_type::iterator backend_iterator_type;
    typedef std::pair<const K,V> entry_type;
    typedef _mqmap_iterator<K,V> iterator_type;
};

template<typename K, typename V> class _mqmap_iterator : private _mqmap_types<K,V>::backend_iterator_type {
private:
    typedef _mqmap_types<K,V> types;
    typedef typename types::backend_iterator_type base_type;
    typedef typename types::entry_type entry_type;

    base_type& base() {
        return *static_cast<base_type*>(this);
    }

    template<typename K_, typename V_, int N> friend class mqmap;

public:
    _mqmap_iterator<K,V>(const base_type &base) : base_type(base) {}
    _mqmap_iterator<K,V>() {}

    entry_type *operator->() {
        return reinterpret_cast<entry_type*>(&(*base()));
    }

    entry_type &operator*() {
        return *operator->();
    }

    _mqmap_iterator<K,V> &operator++() {
        base() = base()->second._mqmap_next.base();
        return *this;
    }

    _mqmap_iterator<K,V> &operator--() {
        base() = base()->second._mqmap_prev.base();
        return *this;
    }

    _mqmap_iterator<K,V> operator++(int _) {
        _mqmap_iterator old = *this;
        (*this)++;
        return old;
    }

    _mqmap_iterator<K,V> operator--(int _) {
        _mqmap_iterator old = *this;
        (*this)--;
        return old;
    }
};

template<typename K, typename V, int N> class mqmap {
private:
    typedef _mqmap_types<K,V> types;

public:
    typedef typename types::iterator_type iterator;

private:
    typedef typename types::backend_entry_type backend_entry_type;
    typedef typename types::entry_type entry_type;
    typedef typename types::backend_type backend_type;
    typedef typename types::backend_iterator_type backend_iterator_type;

    backend_type backend;
    iterator front_iter[N];
    iterator back_iter[N];

public:
    mqmap() {
        for (int i=0; i<N; i++) {
             front_iter[i] = none();
             back_iter[i] = none();
        }
    }

    iterator front(int n) const {
        return front_iter[n];
    }

    iterator back(int n) const {
        return back_iter[n];
    }

    iterator none() {
        return backend.end();
    }

    iterator find(const K& key) {
        return backend.find(key);
    }

    iterator insert(const K& key) {
        backend_iterator_type pos = backend.lower_bound(key);
        if (pos != backend.end() && pos->first == key) {
            return pos;
        } else {
            return backend.insert(pos, std::make_pair(key, backend_entry_type(backend.end())));
        }
    }

    V& operator[](const K& key) {
        return insert(key)->second;
    }

    int get_queue(iterator pos) {
        backend_entry_type &entry = pos.base()->second;
        return entry._mqmap_queue;
    }

    void pop(iterator pos) {
        if (pos == none())
            return;
        backend_entry_type &entry = pos.base()->second;
        if (entry._mqmap_queue != -1) {
            if (entry._mqmap_prev != none())
                entry._mqmap_prev.base()->second._mqmap_next = entry._mqmap_next;
            else
                front_iter[entry._mqmap_queue] = entry._mqmap_next;
            if (entry._mqmap_next != none())
                entry._mqmap_next.base()->second._mqmap_prev = entry._mqmap_prev;
            else
                back_iter[entry._mqmap_queue] = entry._mqmap_prev;
            entry._mqmap_queue = -1;
            entry._mqmap_next = none();
            entry._mqmap_prev = none();
        }
    }

    void push_front(int n, iterator pos) {
        if (pos == none())
            return;
        pop(pos);
        backend_entry_type &entry = pos.base()->second;
        entry._mqmap_next = front_iter[n];
        entry._mqmap_prev = none();
        front_iter[n].base()->second._mqmap_prev = pos;
        front_iter[n] = pos;
    }

    void push_back(int n, iterator pos) {
        if (pos == none())
            return;
        pop(pos);
        backend_entry_type &entry = pos.back()->second;
        entry._mqmap_prev = back_iter[n];
        entry._mqmap_next = none();
        back_iter[n].base()->second._mqmap_next = pos;
        back_iter[n] = pos;
    }

    void erase(iterator pos) {
        pop(pos);
        backend.erase(pos);
    }
};

#endif
