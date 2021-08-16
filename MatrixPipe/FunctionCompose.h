//
// Created by slava on 16.08.2021.
//

#ifndef MATRIXPIPE_FUNCTIONCOMPOSE_H
#define MATRIXPIPE_FUNCTIONCOMPOSE_H

#include <functional>

namespace detail {
    template<class T, int>
    struct InhStruct : public T {
        using T::T;

        template<class B>
        explicit InhStruct(B &&b) : T(std::forward<B>(b)) {}
    };

    template<class T>
    using FstFun = InhStruct<T, 1>;

    template<class T>
    using SecFun = InhStruct<T, 2>;
}

template<class F1, class F2>
class FunctionCompose : private detail::FstFun<F1>, private detail::SecFun<F2> {
    detail::FstFun<F1> &inner_function1() {
        return *this;
    }

    detail::SecFun<F2> &inner_function2() {
        return *this;
    }

public:
    template<class A, class B>
    FunctionCompose(A &&f1, B &&f2) : detail::FstFun<F1>(std::forward<A>(f1)),
                                      detail::SecFun<F2>(std::forward<B>(f2)) {}

    template<class... Args>
    decltype(auto) operator()(Args &&... args) {
        return std::invoke(inner_function1(), std::invoke(inner_function2(), args...));
    }
};

template<class F1, class F2>
FunctionCompose<std::decay_t<F1>, std::decay_t<F2>> compose(F1 &&f1, F2 &&f2) {
    return {std::forward<F1>(f1), std::forward<F2>(f2)};
}


#endif //MATRIXPIPE_FUNCTIONCOMPOSE_H
