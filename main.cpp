#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>
#include <random>
#include <algorithm>
#include <fstream>

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
        return;
    }

    for(const auto& block : task.blocks) {
        multiplyBlock(A, B, C, block.first, block.second, blockSize);
    }
}

std::vector<ComputeTask> createTasks(int rows, int cols, int numThreads, int blockSize = 2) {
    std::vector<ComputeTask> tasks(numThreads);

    int blocksRows = (rows + blockSize - 1) / blockSize;
    int blocksCols = (cols + blockSize - 1) / blockSize;
    int totalBlocks = blocksRows * blocksCols;

    for(int blockIndex = 0; blockIndex < totalBlocks; blockIndex++) {
        int blockRow = blockIndex / blocksCols;
        int blockCol = blockIndex % blocksCols;
        int taskIndex = blockIndex % numThreads;

        tasks[taskIndex].addBlock(blockRow, blockCol);
    }

    return tasks;
}

void multiplyWithThreads(const Matrix& A, const Matrix& B, Matrix& C,
                         const std::vector<ComputeTask>& tasks, int blockSize = 2) {
    std::vector<std::thread> threads;

    for(int i = 0; i < tasks.size() - 1; i++) {
        threads.emplace_back(processTask, std::cref(tasks[i]), std::cref(A), std::cref(B), std::ref(C), blockSize);
    }

    processTask(tasks[tasks.size() - 1], A, B, C, blockSize);

    for(auto& thread : threads) {
        thread.join();
    }
}

template<typename Func>
long long measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

void multiplyWithAsync(const Matrix& A, const Matrix& B, Matrix& C,
                       const std::vector<ComputeTask>& tasks, int blockSize = 2) {
    std::vector<std::future<void>> futures;

    for(int i = 0; i < tasks.size(); i++) {
        futures.push_back(
                std::async(std::launch::async, processTask,
                           std::cref(tasks[i]), std::cref(A), std::cref(B), std::ref(C), blockSize)
        );
    }

    for(auto& future : futures) {
        future.wait();
    }
}

void compareMethods(int matrixSize, int numThreads, int blockSize, std::ofstream& outputFile) {
    Matrix A(matrixSize, matrixSize);
    Matrix B(matrixSize, matrixSize);
    Matrix C_seq(matrixSize, matrixSize);
    Matrix C_threads(matrixSize, matrixSize);
    Matrix C_async(matrixSize, matrixSize);

    A.fillRandom(1, 100);
    B.fillRandom(1, 100);

    auto tasks = createTasks(matrixSize, matrixSize, numThreads, blockSize);

    // 1. Последовательное умножение
    long long timeSeq = measureTime([&]() {
        sequentialMultiply(A, B, C_seq);
    });

    // 2. Многопоточное с std::thread
    long long timeThreads = measureTime([&]() {
        multiplyWithThreads(A, B, C_threads, tasks, blockSize);
    });

    // 3. Асинхронное с std::async
    long long timeAsync = measureTime([&]() {
        multiplyWithAsync(A, B, C_async, tasks, blockSize);
    });

    // Проверка корректности
    bool threadsCorrect = true;
    bool asyncCorrect = true;

    for (int i = 0; i < matrixSize; i++) {
        for (int j = 0; j < matrixSize; j++) {
            if (C_threads(i, j) != C_seq(i, j)) threadsCorrect = false;
            if (C_async(i, j) != C_seq(i, j)) asyncCorrect = false;
        }
    }

    // Запись в файл
    outputFile << matrixSize << "x" << matrixSize << ","
               << numThreads << "," << blockSize << ","
               << timeSeq << "," << timeThreads << "," << timeAsync << ","
               << (double)timeSeq / timeThreads << "," << (double)timeSeq / timeAsync << ","
               << (threadsCorrect ? "YES" : "NO") << "," << (asyncCorrect ? "YES" : "NO") << std::endl;

    // Вывод в консоль для прогресса
    std::cout << "Матрица " << matrixSize << "x" << matrixSize
              << ", Потоков: " << numThreads << ", Блок: " << blockSize
              << " -> Seq: " << timeSeq << " мкс, Threads: " << timeThreads
              << " мкс, Async: " << timeAsync << " мкс" << std::endl;
}

void testThreadScaling(int matrixSize, int blockSize, std::ofstream& outputFile) {
    std::cout << "\n--- Масштабирование по потокам (матрица " << matrixSize << "x" << matrixSize << ") ---" << std::endl;

    std::vector<int> threadCounts = {1, 2, 4, 8, 16};

    for (int threads : threadCounts) {
        compareMethods(matrixSize, threads, blockSize, outputFile);
    }
}

void testBlockSizeScaling(int matrixSize, int numThreads, std::ofstream& outputFile) {
    std::cout << "\n--- Масштабирование по размеру блока (матрица " << matrixSize << "x" << matrixSize << ", " << numThreads << " потоков) ---" << std::endl;

    std::vector<int> blockSizes = {2, 4, 8, 16, 32, 64, 128};

    for (int blockSize : blockSizes) {
        compareMethods(matrixSize, numThreads, blockSize, outputFile);
    }
}

int main() {
    std::cout << "=== Тестирование производительности ===" << std::endl;

    // Открываем файл для записи
    std::ofstream outputFile("compare.txt");
    if (!outputFile.is_open()) {
        std::cerr << "Ошибка открытия файла!" << std::endl;
        return 1;
    }

    // Заголовок CSV
    outputFile << "MatrixSize,Threads,BlockSize,SequentialTime,ThreadsTime,AsyncTime,ThreadsSpeedup,AsyncSpeedup,ThreadsCorrect,AsyncCorrect" << std::endl;

    // ТЕСТ 1: Масштабирование по потокам для разных размеров матриц
    std::cout << "\n=== ТЕСТ 1: Зависимость от количества потоков ===" << std::endl;

    std::vector<int> matrixSizes = {100, 200, 500};
    for (int size : matrixSizes) {
        int optimalBlockSize = (size >= 500) ? 32 : (size >= 200 ? 16 : 8);
        testThreadScaling(size, optimalBlockSize, outputFile);
    }

    // ТЕСТ 2: Масштабирование по размеру блока для разных конфигураций
    std::cout << "\n=== ТЕСТ 2: Зависимость от размера блока ===" << std::endl;

    // Для маленькой матрицы
    testBlockSizeScaling(100, 4, outputFile);

    // Для средней матрицы
    testBlockSizeScaling(200, 8, outputFile);

    // Для большой матрицы
    testBlockSizeScaling(500, 8, outputFile);

    outputFile.close();
    std::cout << "\nРезультаты сохранены в файл compare.txt" << std::endl;
    std::cout << "Формат CSV: MatrixSize,Threads,BlockSize,SequentialTime,ThreadsTime,AsyncTime,ThreadsSpeedup,AsyncSpeedup,ThreadsCorrect,AsyncCorrect" << std::endl;

    return 0;
}