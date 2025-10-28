#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>


void workerFunction(int id, int sleepTimeMs) {
    std::cout << "Поток " << id << " начал работу!" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
    std::cout << "Поток " << id << " завершил работу!" << std::endl;
}

void creatingThreads() {
    std::vector<std::thread> threads;

    threads.reserve(3);
    for(int i = 0; i < 3; i++) {
        threads.emplace_back(workerFunction, i, (i + 1) * 10000);
    }

    std::for_each(threads.begin(), threads.end(), [](std::thread &t) {
        t.join();
    });

    std::cout << "Потоки завершили работу!" << std::endl;

}

int square(int number) {
    std::cout << "squaring " << number << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return number * number;
}

void asyncCalculations() {
    std::vector<std::future<int>> squares;
    squares.reserve(10);
    for(int i = 0; i < 10; i++) {
        squares.push_back(std::async(std::launch::async, square, i));
    }
    for(int i = 0; i < squares.size(); i++) {
        std::cout << i << "^2 = "<< squares.at(i).get() << std::endl;
    }
}

int calculateSumInInterval(std::vector<int> &hugeVector, int begin, int end) {
    int sum = 0;
    for(int i = begin; i < end; i++) {
        sum += hugeVector[i];
    }
    return sum;
}

int calculateMultithreaded(std::vector<int> &hugeVector) {
    std::vector<std::future<int>> sums;
    sums.push_back(std::async(std::launch::async, calculateSumInInterval, std::ref(hugeVector), 0, 250000));
    sums.push_back(std::async(std::launch::async, calculateSumInInterval, std::ref(hugeVector), 250000, 500000));
    sums.push_back(std::async(std::launch::async, calculateSumInInterval, std::ref(hugeVector), 500000, 750000));
    sums.push_back(std::async(std::launch::async, calculateSumInInterval, std::ref(hugeVector), 750000, 999999));
    int totalSum = 0;
    for(auto &future : sums) {
        totalSum += future.get();
    }
    return totalSum;
}

int calculateSinglethreaded(std::vector<int> &hugeVector) {
    return calculateSumInInterval(std::ref(hugeVector),0, 999999);
}

//Сумма с использованием 1 потока вычислена за 1369µsмкс и равна 1782293665
//Сумма с использованием 4 потоков вычислена за 626µsмкс и равна 1782293665
void calculationsComparing() {
    std::vector<int> hugeVector;
    hugeVector.reserve(1000000);
    for(int i = 0; i < 1000000; i++) {
        hugeVector.push_back(i);
    }
    auto start = std::chrono::high_resolution_clock::now();
    int resultSinglethreaded = calculateSinglethreaded(std::ref(hugeVector));
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Сумма с использованием 1 потока вычислена за " << duration << "мкс и равна " << resultSinglethreaded << std::endl;

    start = std::chrono::high_resolution_clock::now();
    int resultMultihreaded = calculateMultithreaded(std::ref(hugeVector));
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Сумма с использованием 4 потоков вычислена за " << duration << "мкс и равна " << resultMultihreaded << std::endl;
}

void exceptionsMultithreadedHandler() {

}

int main() {
    calculationsComparing();
    return 0;
}


