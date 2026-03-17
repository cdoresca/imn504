/*
 *	Factory class for managing resources
 *
 * throw exception logic_error if creation fails
 *
 */
#ifndef _RESOURCE_MGR
#define _RESOURCE_MGR

#include "Logger/ImGuiLogger.h"
#include <iostream>
#include <map>

template <typename T>
class Resource_mgr {
public:
    Resource_mgr();
    ~Resource_mgr();

    T *get(std::string a);
    void insert(std::string a, T *elt);
    void release(std::string a);
    T *find(std::string a);
    template <typename R>
    R *get(std::string a);
    T *get(int a);
    int size();
    T *nextObject(std::string a);

private:
    std::map<std::string, std::pair<T *, int>> rscmap;
};
template <typename T>
Resource_mgr<T>::Resource_mgr() {
}

template <typename T>
Resource_mgr<T>::~Resource_mgr() {
    for (typename std::map<std::string, std::pair<T *, int>>::iterator it = rscmap.begin(); it != rscmap.end(); it++) {
        if ((*it).second.first != NULL) {
            delete (*it).second.first;
            (*it).second.first = NULL;
        }
    }

    rscmap.clear();
}

template <typename T>
int Resource_mgr<T>::size() {
    return (int)rscmap.size();
}

template <typename T>
template <typename R>
R *Resource_mgr<T>::get(std::string a) {
    R *to_ret = NULL;
    typename std::map<std::string, std::pair<T *, int>>::iterator it = rscmap.find(a);
    if (it == rscmap.end()) {
        try {
            LOG_TRACE << "Creating New " << a << std::endl;
            to_ret = new R(a);

        } catch (const std::exception &e) {
            LOG_WARNING << "ERROR RSCMGR : " << e.what() << std::endl;
            return NULL;
        }
        rscmap.insert(std::make_pair(a, std::make_pair(to_ret, 1)));
    } else {
        to_ret = dynamic_cast<R *>((*it).second.first);
        if (!to_ret)
            LOG_WARNING << "ERROR RSCMGR : Type conversion " << std::endl;
        (*it).second.second++;
    }

    return to_ret;
}

template <typename T>
T *Resource_mgr<T>::get(std::string a) {
    T *to_ret = NULL;
    typename std::map<std::string, std::pair<T *, int>>::iterator it = rscmap.find(a);
    if (it == rscmap.end()) {
        try {
            LOG_TRACE << "Creating New " << a << std::endl;
            to_ret = new T(a);
        } catch (const std::exception &e) {
            LOG_WARNING << "ERROR RSCMGR : " << e.what() << std::endl;
            return NULL;
        }
        rscmap.insert(make_pair(a, std::make_pair(to_ret, 1)));
    } else {
        to_ret = (*it).second.first;
        (*it).second.second++;
    }

    return to_ret;
}
template <typename T>
T *Resource_mgr<T>::get(int a) {
    T *to_ret = NULL;
    if (a >= (int)rscmap.size())
        return to_ret;

    typename std::map<std::string, std::pair<T *, int>>::iterator it = rscmap.begin();
    for (int i = 0; i < a; i++)
        ++it;
    to_ret = (*it).second.first;

    return to_ret;
}

template <typename T>
T *Resource_mgr<T>::nextObject(std::string a) {

    typename std::map<std::string, std::pair<T *, int>>::iterator it_elt = rscmap.find(a);
    if (it_elt != rscmap.end())
        it_elt = std::next(it_elt);
    if (it_elt == rscmap.end())
        it_elt = rscmap.begin();

    return (*it_elt).second.first;
}

template <typename T>
void Resource_mgr<T>::insert(std::string a, T *elt) {
    typename std::map<std::string, std::pair<T *, int>>::std::iterator it = rscmap.find(a);
    if (it == rscmap.end())
        rscmap.insert(make_pair(a, make_pair(elt, 0)));
}

template <typename T>
T *Resource_mgr<T>::find(std::string a) {
    T *to_ret = NULL;
    typename std::map<std::string, std::pair<T *, int>>::iterator it = rscmap.find(a);
    if (it != rscmap.end())
        to_ret = (*it).second.first;

    return to_ret;
}
template <typename T>
void Resource_mgr<T>::release(std::string a) {
    typename std::map<std::string, std::pair<T *, int>>::iterator it = rscmap.find(a);
    if (it != rscmap.end()) {
        (*it).second.second--;
        if ((*it).second.second <= 0) {
            delete (*it).second.first;
            rscmap.erase(it);
        }
    }
}

#endif
