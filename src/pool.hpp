template <typename T, size_t SIZE>
struct pool_t {

    T *alloc() {
        if (trash_top) {
            return new(&trash[--trash_top])T;
        }
        else {
            return new(&pool[pool_top++])T;
        }
    }

    void del(T* pos) {

    }

    T pool[SIZE];
    T *trash[SIZE];
    int pool_top = 0, trash_top = 0;
};