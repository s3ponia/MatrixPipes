#include <iostream>
#include <vector>
#include <string>
#include <memory_resource>
#include <sstream>
#include <functional>
#include <fstream>

#include "Matrix.h"
#include "FunctionCompose.h"
#include "FlatMap.h"
#include "Pipe.h"

template<class T>
using PmrMatrix = Matrix<T, std::pmr::polymorphic_allocator<T>>;

template<class T>
using PmrPipe = Pipe<T, std::pmr::polymorphic_allocator<T>>;

using MatrixFunction = std::function<PmrMatrix<double>(const PmrMatrix<double> &)>;

template<class It>
void split(const std::string &s, It outputIt) {
    std::istringstream ss(s);
    std::string in;

    while (ss >> in) {
        *outputIt++ = in;
    }
}

PmrMatrix<double> matrix_mul(const PmrMatrix<double> &lhs, const PmrMatrix<double> &rhs) {

    PmrMatrix<double> res(lhs);
    res *= rhs;
    return res;
}

PmrMatrix<double> matrix_add(const PmrMatrix<double> &lhs, const PmrMatrix<double> &rhs) {
    PmrMatrix<double> res(lhs);
    res += rhs;
    return res;
}

PmrMatrix<double> vec_dot_vec(const PmrMatrix<double> &lhs, const PmrMatrix<double> &rhs) {
    return PmrMatrix<double>(1, 1, lhs.dot(rhs));
}

PmrMatrix<double> read_matrix(std::istream &is) {
    std::string in;
    Matrix<double>::size_type element_count;
    std::vector<std::vector<std::string>> tempSMatrix;

    std::vector<std::string> splitBySpace;
    std::getline(is, in);
    split(in, std::back_inserter(splitBySpace));
    element_count = splitBySpace.size();
    splitBySpace.shrink_to_fit();
    tempSMatrix.push_back(splitBySpace);

    splitBySpace.clear();

    while (std::getline(is, in)) {
        split(in, std::back_inserter(splitBySpace));
        if (splitBySpace.size() != element_count) {
            throw std::runtime_error("Wrong file format.");
        }
        tempSMatrix.push_back(splitBySpace);
        splitBySpace.clear();
    }

    if (tempSMatrix.empty()) {
        throw std::runtime_error("Empty file.");
    }

    PmrMatrix<double> result(tempSMatrix.size(), tempSMatrix[0].size());

    for (Matrix<double>::size_type i = 0; i < result.get_row_count(); ++i) {
        for (Matrix<double>::size_type j = 0; j < result.get_element_count(); ++j) {
            result(i, j) = std::stod(tempSMatrix[i][j]);
        }
    }

    return result;
}

void print_matrix(std::ostream &os, const PmrMatrix<double> &m) {
    for (Matrix<double>::size_type i = 0; i < m.get_row_count(); ++i) {
        for (Matrix<double>::size_type j = 0; j < m.get_element_count(); ++j) {
            os << m(i, j) << ' ';
        }
        os << '\n';
    }
}

MatrixFunction decode_function(std::istream &is) {
    using function_pointer = PmrMatrix<double>(*)(const PmrMatrix<double> &, const PmrMatrix<double> &);
    static FlatMap<std::string_view, function_pointer> map(
            {
                    {std::string_view{"mat_mul_vec"}, &matrix_mul},
                    {std::string_view{"mat_mul_mat"}, &matrix_mul},
                    {std::string_view{"vec_mul_mat"}, &matrix_mul},
                    {std::string_view{"vec_add_vec"}, &matrix_add},
                    {std::string_view{"mat_add_vec"}, &matrix_add},
                    {std::string_view{"mat_add_mat"}, &matrix_add},
                    {std::string_view{"vec_dot_vec"}, &vec_dot_vec}
            }
    );
    std::string function_name;
    is >> function_name;


    std::string file_name;
    is >> file_name;

    std::ifstream matrix_file(file_name);
    if (!matrix_file.is_open()) {
        throw std::runtime_error{"Cannot open matrix_file"};
    }

    const auto matrix = read_matrix(matrix_file);
    std::string_view fun_name_view = function_name;
    if (fun_name_view.substr(0, 3) != fun_name_view.substr(fun_name_view.size() - 3)
        && ((fun_name_view.substr(0, 3) == "vec") == matrix.is_vector())) {
        return [matrix, function = map[function_name]](const PmrMatrix<double> &m) {
            return function(matrix, m);
        };
    }

    return [matrix, function = map[function_name]](const PmrMatrix<double> &m) {
        return function(m, matrix);
    };
}

PmrPipe<MatrixFunction> config(std::istream &is) {
    PmrPipe<MatrixFunction> res;

    while (!is.eof()) {
        res.push_back(decode_function(is));
    }

    return res;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <config file> <input matrix file> <output matrix file>\n";
        return EXIT_FAILURE;
    }

    std::string config_file_name = argv[1];
    std::string input_file_name = argv[2];
    std::string output_file_name = argv[3];

    std::ifstream config_file(config_file_name);
    std::ifstream input_file(input_file_name);
    std::ofstream output_file(output_file_name);

    const auto input = read_matrix(input_file);
    const auto pipe = config(config_file);
    print_matrix(output_file, pipe(input));

    return 0;
}
