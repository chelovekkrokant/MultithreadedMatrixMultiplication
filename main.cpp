#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>
#include <random>
#include <algorithm>

enum ParallelMethods {
    THREADS,
    ASYNC
};

class Matrix {
private:
    std::vector<std::vector<int>> data;
    int rows;
    int cols;

public:
    Matrix(int r, int c) : rows(r), cols(c) {
        data.resize(rows, std::vector<int>(cols, 0));
    }

    Matrix(int size) : rows(size), cols(size) {
        data.resize(rows, std::vector<int>(cols, 0));
    }

    int& operator()(int i, int j) { return data[i][j]; }
    const int& operator()(int i, int j) const { return data[i][j]; }

    int getRows() const { return rows; }
    int getCols() const { return cols; }

    void fillRandom(int minVal = 0, int maxVal = 255) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(minVal, maxVal);

        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < cols; j++) {
                data[i][j] = dis(gen);
            }
        }
    }

    void print(const std::string& name = "Matrix") const {
        std::cout << name << " (" << rows << "x" << cols << "):\n";
        for(int i = 0; i < std::min(rows, 5); i++) {
            for(int j = 0; j < std::min(cols, 5); j++) {
                std::cout << data[i][j] << "\t";
            }
            if (cols > 5) std::cout << "...";
            std::cout << "\n";
        }
        if (rows > 5) std::cout << "...\n";
        std::cout << std::endl;
    }
};

struct ComputeTask {
    std::vector<std::pair<int, int>> blocks;

    void addBlock(int row, int col) {
        blocks.emplace_back(row, col);
    }
};

void sequentialMultiply(const Matrix& A, const Matrix& B, Matrix& C) {
    for (int i = 0; i < A.getRows(); i++) {
        for(int j = 0; j < B.getCols(); j++) {
            C(i, j) = 0;
            for (int k = 0; k < A.getCols(); k++) {
                C(i, j) += A(i, k) * B(k, j);
            }
        }
    }
}

void multiplyBlock(const Matrix& A, const Matrix& B, Matrix& C, int blockRow, int blockCol, int blockSize = 2) {
    int startRow = blockRow * blockSize;
    int startCol = blockCol * blockSize;

    int endRow = std::min(startRow + blockSize, C.getRows());
    int endCol = std::min(startCol + blockSize, C.getCols());

    std::cout << "Вычисление блока (" << blockRow << "," << blockCol << "): "
              << "строки " << startRow << "-" << endRow-1 << ", "
              << "столбцы " << startCol << "-" << endCol-1 << std::endl;

    for (int i = startRow; i < endRow; i++) {
        for(int j = startCol; j < endCol; j++) {
            C(i, j) = 0;
            for(int k = 0; k < A.getCols(); k++) {
                C(i, j) += A(i,k) * B(k, j);
            }
        }
    }
}

void processTask(const ComputeTask& task, const Matrix& A, const Matrix& B, Matrix& C, int blockSize = 2) {
    if (task.blocks.empty()) {
        std::cout << "Предупреждение: пустая задача!" << std::endl;
        return;
    }

    for(const auto& block : task.blocks) {
        multiplyBlock(A, B, C, block.first, block.second, blockSize);
    }
}

void initializeMatrices(Matrix& A, Matrix& B, Matrix& C) {
    A.fillRandom();
    B.fillRandom();
}

int main() {
    std::cout << "=== Шаг 0: Подготовка структур данных ===" << std::endl;

    Matrix A(4, 4);
    Matrix B(4, 4);
    Matrix C(4, 4);

    initializeMatrices(A, B, C);

    A.print("Matrix A");
    B.print("Matrix B");
    C.print("Matrix C (result)");

    ComputeTask task;
    task.addBlock(0, 0);
    task.addBlock(0, 1);
    task.addBlock(1, 0);
    task.addBlock(1, 1);


    std::cout << "Создана задача с " << task.blocks.size() << " блоками." << std::endl;

    return 0;

}