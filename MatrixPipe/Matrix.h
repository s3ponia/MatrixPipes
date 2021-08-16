//
// Created by slava on 16.08.2021.
//

#ifndef MATRIXPIPE_MATRIX_H
#define MATRIXPIPE_MATRIX_H

#include <memory>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <type_traits>

namespace detail {
    template<class It, class Alloc>
    void destroy(It first, It last, Alloc &a) {
        for (; first != last; ++first)
            std::allocator_traits<Alloc>::destroy(a, std::addressof(*first));
    }

    template<class It, class Alloc>
    void uninitialized_default_construct(It first, It last, Alloc &a) {
        It current = first;
        try {
            for (; current != last; ++current) {
                std::allocator_traits<Alloc>::construct(a, std::addressof(*current));
            }
        } catch (...) {
            (destroy)(first, current, a);
            throw;
        }
    }

    template<class It, class T, class Alloc>
    void uninitialized_fill(It first, It last, const T &value, Alloc &a) {
        It current = first;
        try {
            for (; current != last; ++current) {
                std::allocator_traits<Alloc>::construct(a, std::addressof(*current), value);
            }
        } catch (...) {
            (destroy)(first, current, a);
            throw;
        }
    }

    template<class InputIt, class NoThrowForwardIt, class Alloc>
    void uninitialized_copy(InputIt first, InputIt last,
                                        NoThrowForwardIt d_first, Alloc &a) {
        NoThrowForwardIt current = d_first;
        try {
            for (; first != last; ++first, (void) ++current) {
                std::allocator_traits<Alloc>::construct(a, std::addressof(*current), *first);
            }
        } catch (...) {
            (destroy)(first, current, a);
            throw;
        }
    }
}

struct MatrixException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

template<class T, class Alloc = std::allocator<T>>
class Matrix : private Alloc {
    using AllocTraits = std::allocator_traits<Alloc>;

    void clear() {
        detail::destroy(data, data + size(), allocator());
        AllocTraits::deallocate(allocator(), data, size());
        element_count = 0;
        row_count = 0;
        data = nullptr;
    }

    void reset() {
        element_count = row_count = 0;
        data = nullptr;
    }

    Alloc &allocator() {
        return static_cast<Alloc &>(*this);
    }

    const Alloc &allocator() const {
        return static_cast<const Alloc &>(*this);
    }

public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = typename AllocTraits::pointer;
    using const_pointer = typename AllocTraits::const_pointer;

    Matrix() = default;

    Matrix(size_type row_count, size_type element_count, const Alloc &alloc = Alloc()) : Alloc(alloc),
                                                                                         row_count(row_count),
                                                                                         element_count(element_count) {
        size_type sz = row_count * element_count;
        RAII_memory memory{
                AllocTraits::allocate(allocator(), sz),
                sz,
                *this
        };

        detail::uninitialized_default_construct(memory.data + 0, memory.data + sz, allocator());

        std::swap(data, memory.data);
    }

    Matrix(size_type row_count, size_type element_count, const T &value,
           const Alloc &alloc = Alloc()) : Alloc(alloc),
                                           row_count(row_count),
                                           element_count(element_count) {
        size_type sz = row_count * element_count;
        RAII_memory memory{
                AllocTraits::allocate(allocator(), sz),
                sz,
                *this
        };

        detail::uninitialized_fill(memory.data + 0, memory.data + sz, value, allocator());

        std::swap(data, memory.data);
    }

    Matrix(const Matrix &m) :
            Alloc(AllocTraits::select_on_container_copy_construction(m.allocator())),
            row_count(m.row_count),
            element_count(m.element_count) {
        size_type sz = row_count * element_count;
        RAII_memory memory{
                AllocTraits::allocate(allocator(), sz),
                sz,
                *this
        };

        detail::uninitialized_copy(m.data + 0, m.data + sz, memory.data, allocator());

        std::swap(data, memory.data);
    }

    Matrix(Matrix &&m) noexcept: Alloc(std::move(m.allocator())),
                                 row_count(m.row_count),
                                 element_count(m.element_count),
                                 data(m.data) {
        m.reset();
    }

    Matrix &operator=(const Matrix &m) {
        if (this == &m) {
            return *this;
        }

        size_type sz = m.row_count * m.element_count;
        RAII_memory memory{
                {},
                sz,
                *this
        };

        if constexpr (poca()) {
            memory.data = AllocTraits::allocate(m.allocator(), sz);
            detail::uninitialized_copy(m.data, m.data + m.size(), memory.data, m.allocator());
            clear();
            allocator() = m.allocator();
        } else {
            memory.data = AllocTraits::allocate(allocator(), sz);
            detail::uninitialized_copy(m.data, m.data + m.size(), memory.data, allocator());
            clear();
        }

        row_count = m.row_count;
        element_count = m.element_count;
        std::swap(data, memory.data);

        return *this;
    }

    Matrix &operator=(Matrix &&m) noexcept {
        if constexpr (pocma()) {
            allocator() = std::move(m.allocator());
        }

        data = m.data;
        row_count = m.row_count;
        element_count = m.element_count;
        m.reset();

        return *this;
    }

    reference operator()(size_type i1, size_type i2) {
        return data[i1 * element_count + i2];
    }

    const_reference operator()(size_type i1, size_type i2) const {
        return data[i1 * element_count + i2];
    }

    [[nodiscard]] size_type size() const noexcept {
        return element_count * row_count;
    }

    [[nodiscard]] size_type get_row_count() const noexcept {
        return row_count;
    }

    [[nodiscard]] size_type get_element_count() const noexcept {
        return element_count;
    }

    Matrix &operator+=(const Matrix &m) {
        if (!(m.element_count == element_count && m.row_count == row_count)) {
            throw MatrixException("Matrix size mismatch");
        }

        for (size_type i = 0; i < size(); ++i) {
            data[i] += m.data[i];
        }

        return *this;
    }

    template<class IntType, typename = std::enable_if_t<std::is_arithmetic_v<IntType>>>
    Matrix &operator*=(const IntType &value) {
        for (size_type i = 0; i < size(); ++i) {
            data[i] *= value;
        }

        return *this;
    }

    Matrix &operator*=(const Matrix &m) {
        if (element_count != m.row_count) {
            throw MatrixException("Matrix size mismatch");
        }

        Matrix result(row_count, m.element_count);

        for (size_type i = 0; i < result.row_count; ++i) {
            for (size_type j = 0; j < result.element_count; ++j) {
                value_type res_val{};

                for (size_type r = 0; r < m.row_count; ++r) {
                    res_val += (*this)(i, r) * m(r, j);
                }

                result(i, j) = res_val;
            }
        }

        *this = std::move(result);

        return *this;
    }

    [[nodiscard]] value_type dot(const Matrix &rhs) const {
        if (!(get_element_count() == rhs.get_element_count() && get_row_count() == rhs.get_row_count())) {
            throw MatrixException{"Size mismatch in matrices"};
        }

        if (get_row_count() != 1 && get_element_count() != 1) {
            throw MatrixException{"Matrices have to be vectors"};
        }

        value_type res;

        for (size_type i = 0; i < size(); ++i) {
            res += data[i] * rhs.data[i];
        }

        return res;
    }

    [[nodiscard]] bool is_vector() const noexcept {
        return element_count == 1 || row_count == 1;
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(allocator());
    }

    ~Matrix() {
        clear();
    }

private:
    static constexpr bool pocma() {
        return AllocTraits::propagate_on_container_move_assignment::value;
    }

    static constexpr bool poca() {
        return AllocTraits::propagate_on_container_copy_assignment::value;
    }

    struct RAII_memory {
        pointer data;
        size_type n;
        Alloc &alloc;

        ~RAII_memory() {
            if (data != nullptr)
                AllocTraits::deallocate(alloc, data, n);
        }
    };

    T *data{};
    size_type row_count{};
    size_type element_count{};
};

#endif //MATRIXPIPE_MATRIX_H
