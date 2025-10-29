import matplotlib.pyplot as plt
import csv

def read_results(filename):
    """Чтение результатов из CSV файла"""
    results = []
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        headers = next(reader)  # Пропускаем заголовок

        for row in reader:
            if not row or row[0].startswith('#'):
                continue

            try:
                # Парсим данные
                matrix_size = int(row[0].split('x')[0])  # "100x100" -> 100
                threads = int(row[1])
                block_size = int(row[2])
                seq_time = int(row[3])
                threads_time = int(row[4])
                async_time = int(row[5])
                threads_speedup = float(row[6])
                async_speedup = float(row[7])

                results.append({
                    'matrix_size': matrix_size,
                    'threads': threads,
                    'block_size': block_size,
                    'seq_time': seq_time,
                    'threads_time': threads_time,
                    'async_time': async_time,
                    'threads_speedup': threads_speedup,
                    'async_speedup': async_speedup
                })
            except (ValueError, IndexError) as e:
                print(f"Пропуск строки: {row} - ошибка: {e}")
                continue

    return results

def create_speedup_plots(results):
    """Создание графиков ускорения"""
    # Группируем по размеру матрицы
    matrix_sizes = sorted(set(r['matrix_size'] for r in results))

    for size in matrix_sizes:
        # Фильтруем результаты для текущего размера матрицы
        size_results = [r for r in results if r['matrix_size'] == size]
        size_results.sort(key=lambda x: x['threads'])

        # Подготавливаем данные для графика
        threads = [r['threads'] for r in size_results]
        threads_speedup = [r['threads_speedup'] for r in size_results]
        async_speedup = [r['async_speedup'] for r in size_results]

        # Создаем график
        plt.figure(figsize=(10, 6))

        # Графики ускорения
        plt.plot(threads, threads_speedup, 'bo-', linewidth=2, markersize=8, label='std::thread')
        plt.plot(threads, async_speedup, 'ro-', linewidth=2, markersize=8, label='std::async')

        # Идеальное ускорение
        plt.plot(threads, threads, 'g--', linewidth=1, label='Идеальное ускорение', alpha=0.7)

        # Настройки графика
        plt.title(f'Зависимость ускорения от количества потоков\nМатрица {size}×{size}', fontsize=14, fontweight='bold')
        plt.xlabel('Количество потоков')
        plt.ylabel('Ускорение (раз)')
        plt.legend()
        plt.grid(True, alpha=0.3)

        # Добавляем подписи значений
        for i, (t, ts, ass) in enumerate(zip(threads, threads_speedup, async_speedup)):
            plt.annotate(f'{ts:.2f}x', (t, ts), xytext=(0, 10), textcoords='offset points',
                         ha='center', fontsize=9, color='blue')
            plt.annotate(f'{ass:.2f}x', (t, ass), xytext=(0, -15), textcoords='offset points',
                         ha='center', fontsize=9, color='red')

        plt.tight_layout()
        plt.savefig(f'speedup_matrix_{size}.png', dpi=300, bbox_inches='tight')
        plt.show()

def create_comparison_plot(results):
    """Сравнительный график всех размеров матриц"""
    matrix_sizes = sorted(set(r['matrix_size'] for r in results))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    colors = ['blue', 'red', 'orange', 'purple', 'brown']

    for i, size in enumerate(matrix_sizes):
        size_results = [r for r in results if r['matrix_size'] == size]
        size_results.sort(key=lambda x: x['threads'])

        threads = [r['threads'] for r in size_results]
        threads_speedup = [r['threads_speedup'] for r in size_results]
        async_speedup = [r['async_speedup'] for r in size_results]
        color = colors[i % len(colors)]

        # Левый график - std::thread
        ax1.plot(threads, threads_speedup, 'o-', linewidth=2, markersize=6,
                 label=f'{size}×{size}', color=color)

        # Правый график - std::async
        ax2.plot(threads, async_speedup, 's-', linewidth=2, markersize=6,
                 label=f'{size}×{size}', color=color)

    # Настройка графиков
    for ax, title in zip([ax1, ax2], ['std::thread', 'std::async']):
        ax.set_title(title, fontsize=14, fontweight='bold')
        ax.set_xlabel('Количество потоков')
        ax.set_ylabel('Ускорение (раз)')
        ax.legend()
        ax.grid(True, alpha=0.3)

        # Идеальное ускорение
        max_threads = max(r['threads'] for r in results)
        ideal_threads = list(range(1, max_threads + 1))
        ax.plot(ideal_threads, ideal_threads, '--', color='black', alpha=0.5, label='Идеальное')

    plt.suptitle('Сравнение ускорения для разных размеров матриц', fontsize=16, fontweight='bold')
    plt.tight_layout()
    plt.savefig('speedup_comparison.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    # Читаем результаты
    print("Чтение результатов из compare-threads.txt...")
    results = read_results('compare-threads.txt')

    if not results:
        print("Ошибка: не удалось прочитать результаты")
        return

    print(f"Прочитано {len(results)} записей")

    # Создаем графики
    print("Создание графиков...")
    create_speedup_plots(results)
    create_comparison_plot(results)

    print("Готово! Созданы файлы:")
    print("- speedup_matrix_100.png, speedup_matrix_200.png, ...")
    print("- speedup_comparison.png")

if __name__ == "__main__":
    main()