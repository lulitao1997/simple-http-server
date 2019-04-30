#ifndef MIME_HPP_
#define MIME_HPP_

#include <unordered_map>
#include <cstring>

struct view_t {
    const char *l, *r;
    view_t(const char *l, const char *r): l(l), r(r) {}
    view_t(const char *str): l(str), r(str+strlen(str)) {}
    bool operator==(const view_t& a) const {
        if (r-l != a.r-a.l) return false;
        for (int i=0; i<r-l; i++)
            if (l[i] != a.l[i])
                return false;
        return true;
    }
};

namespace std {
    template<>
    struct hash<view_t> {
        size_t operator()(const view_t& k) const {
            size_t ans = 0;
            for (const char *c=k.l; c<k.r; c++)
                ans += ans * 31 + *c;
            return ans;
        }
    };
}

extern std::unordered_map<view_t, const char*> mime_map;

#endif