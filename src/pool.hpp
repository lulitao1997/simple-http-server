#include <cstddef>
#include <utility>
#include <glog/logging.h>

template <typename T, std::size_t SIZE>
struct pool_t {

    template<typename... Args>
    T *alloc(Args&&... args) {
        T *ptr = (trash_top ? trash[--trash_top] : &pool[pool_top++]);
        DLOG(INFO) << "ptr: " << ptr << ", trash_top: " << trash_top;
        memset(ptr, 0, sizeof(T));
        return new(ptr)T(std::forward<Args>(args)...);
    }

    void del(T *pos) {
        pos->~T();
        trash[trash_top++] = pos;
    }

    T pool[SIZE];
    T *trash[SIZE];
    int pool_top = 0, trash_top = 0;
};