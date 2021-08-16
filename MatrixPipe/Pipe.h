//
// Created by slava on 16.08.2021.
//

#ifndef MATRIXPIPE_PIPE_H
#define MATRIXPIPE_PIPE_H

#include <cstddef>
#include <type_traits>
#include <utility>
#include <memory>
#include <vector>

template<class T, class Alloc = std::allocator<T>>
class Pipe {
public:
    explicit Pipe(const Alloc &alloc = Alloc()) : function_objects(alloc) {}

    template<std::size_t N>
    explicit
    Pipe(const T (&arr)[N], const Alloc &alloc = Alloc()) : function_objects(std::begin(arr), std::end(arr), alloc) {}

    template<class A>
    decltype(auto) operator()(A &&value) const {
        std::decay_t<A> temp(std::forward<A>(value));

        for (std::size_t i = 0; i < function_objects.size(); ++i) {
            auto &&function = function_objects[i];
            temp = function(temp);
        }

        return temp;
    }

    template<class A>
    void push_back(A &&a) {
        function_objects.push_back(std::forward<A>(a));
    }

private:
    std::vector<T, Alloc> function_objects;
};


#endif //MATRIXPIPE_PIPE_H
