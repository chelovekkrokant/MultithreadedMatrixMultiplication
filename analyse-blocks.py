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

def create_block_size_plots(results):
    """Создание графиков зависимости ускорения от размера блока"""
    # Группируем по размеру матрицы и количеству потоков
    configurations = set((r['matrix_size'], r['threads']) for r in results)

    for config in sorted(configurations):
        matrix_size, threads = config

        # Фильтруем результаты для текущей конфигурации
        config_results = [r for r in results if r['matrix_size'] == matrix_size and r['threads'] == threads]
        config_results.sort(key=lambda x: x['block_size'])

        # Подготавливаем данные для графика
        block_sizes = [r['block_size'] for r in config_results]
        threads_speedup = [r['threads_speedup'] for r in config_results]
        async_speedup = [r['async_speedup'] for r in config_results]
        threads_time = [r['threads_time'] for r in config_results]
        async_time = [r['async_time'] for r in config_results]

        # Создаем график ускорения
        plt.figure(figsize=(12, 8))

        # График 1: Ускорение от размера блока
        plt.subplot(2, 1, 1)
        plt.plot(block_sizes, threads_speedup, 'bo-', linewidth=2, markersize=8, label='std::thread')
        plt.plot(block_sizes, async_speedup, 'ro-', linewidth=2, markersize=8, label='std::async')

        # Находим оптимальный размер блока
        optimal_threads = block_sizes[threads_speedup.index(max(threads_speedup))]
        optimal_async = block_sizes[async_speedup.index(max(async_speedup))]

        plt.axvline(x=optimal_threads, color='blue', linestyle='--', alpha=0.7,
                    label=f'Оптимум threads: {optimal_threads}')
        plt.axvline(x=optimal_async, color='red', linestyle='--', alpha=0.7,
                    label=f'Оптимум async: {optimal_async}')

        plt.title(f'Зависимость ускорения от размера блока\nМатрица {matrix_size}×{matrix_size}, {threads} потоков',
                  fontsize=14, fontweight='bold')
        plt.xlabel('Размер блока')
        plt.ylabel('Ускорение (раз)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.xscale('log', base=2)
        plt.xticks(block_sizes, block_sizes)

        # Добавляем подписи значений
        for i, (bs, ts, ass) in enumerate(zip(block_sizes, threads_speedup, async_speedup)):
            plt.annotate(f'{ts:.2f}x', (bs, ts), xytext=(0, 10), textcoords='offset points',
                         ha='center', fontsize=8, color='blue')
            plt.annotate(f'{ass:.2f}x', (bs, ass), xytext=(0, -15), textcoords='offset points',
                         ha='center', fontsize=8, color='red')

        # График 2: Время выполнения от размера блока
        plt.subplot(2, 1, 2)
        plt.plot(block_sizes, threads_time, 'bo-', linewidth=2, markersize=6, label='std::thread время')
        plt.plot(block_sizes, async_time, 'ro-', linewidth=2, markersize=6, label='std::async время')

        plt.xlabel('Размер блока')
        plt.ylabel('Время выполнения (мкс)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.xscale('log', base=2)
        plt.xticks(block_sizes, block_sizes)

        # Добавляем подписи значений времени
        for i, (bs, tt, at) in enumerate(zip(block_sizes, threads_time, async_time)):
            plt.annotate(f'{tt/1000:.1f}к', (bs, tt), xytext=(0, 10), textcoords='offset points',
                         ha='center', fontsize=7, color='blue')
            plt.annotate(f'{at/1000:.1f}к', (bs, at), xytext=(0, -15), textcoords='offset points',
                         ha='center', fontsize=7, color='red')

        plt.tight_layout()
        plt.savefig(f'block_size_matrix_{matrix_size}_threads_{threads}.png', dpi=300, bbox_inches='tight')
        plt.show()

        print(f"Матрица {matrix_size}×{matrix_size}, {threads} потоков:")
        print(f"  Оптимальный размер блока (threads): {optimal_threads}, ускорение: {max(threads_speedup):.2f}x")
        print(f"  Оптимальный размер блока (async): {optimal_async}, ускорение: {max(async_speedup):.2f}x")

def create_block_size_comparison(results):
    """Сравнительный график влияния размера блока для всех конфигураций"""
    configurations = set((r['matrix_size'], r['threads']) for r in results)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown']
    markers = ['o', 's', '^', 'D', 'v', '<']

    for i, config in enumerate(sorted(configurations)):
        matrix_size, threads = config
        config_results = [r for r in results if r['matrix_size'] == matrix_size and r['threads'] == threads]
        config_results.sort(key=lambda x: x['block_size'])

        block_sizes = [r['block_size'] for r in config_results]
        threads_speedup = [r['threads_speedup'] for r in config_results]
        async_speedup = [r['async_speedup'] for r in config_results]

        color = colors[i % len(colors)]
        marker = markers[i % len(markers)]

        # Левый график - std::thread
        ax1.plot(block_sizes, threads_speedup, marker=marker, linewidth=2, markersize=6,
                 label=f'{matrix_size}×{matrix_size}, {threads} потоков', color=color)

        # Правый график - std::async
        ax2.plot(block_sizes, async_speedup, marker=marker, linewidth=2, markersize=6,
                 label=f'{matrix_size}×{matrix_size}, {threads} потоков', color=color)

    # Настройка графиков
    for ax, title in zip([ax1, ax2], ['std::thread', 'std::async']):
        ax.set_title(title, fontsize=14, fontweight='bold')
        ax.set_xlabel('Размер блока')
        ax.set_ylabel('Ускорение (раз)')
        ax.legend()
        ax.grid(True, alpha=0.3)
        ax.set_xscale('log', base=2)

        # Добавляем сетку для логарифмической шкалы
        ax.grid(True, which='minor', alpha=0.2)

    plt.suptitle('Сравнение влияния размера блока на ускорение', fontsize=16, fontweight='bold')
    plt.tight_layout()
    plt.savefig('block_size_comparison.png', dpi=300, bbox_inches='tight')
    plt.show()

def analyze_optimal_blocks(results):
    """Анализ оптимальных размеров блоков"""
    print("\n=== АНАЛИЗ ОПТИМАЛЬНЫХ РАЗМЕРОВ БЛОКОВ ===")

    configurations = set((r['matrix_size'], r['threads']) for r in results)

    for config in sorted(configurations):
        matrix_size, threads = config
        config_results = [r for r in results if r['matrix_size'] == matrix_size and r['threads'] == threads]
        config_results.sort(key=lambda x: x['block_size'])

        block_sizes = [r['block_size'] for r in config_results]
        threads_speedup = [r['threads_speedup'] for r in config_results]
        async_speedup = [r['async_speedup'] for r in config_results]

        optimal_threads_block = block_sizes[threads_speedup.index(max(threads_speedup))]
        optimal_async_block = block_sizes[async_speedup.index(max(async_speedup))]

        print(f"Матрица {matrix_size}×{matrix_size}, {threads} потоков:")
        print(f"  std::thread: блок {optimal_threads_block} -> ускорение {max(threads_speedup):.2f}x")
        print(f"  std::async:  блок {optimal_async_block} -> ускорение {max(async_speedup):.2f}x")

        # Анализ эффективности кэша
        if optimal_threads_block > 1:
            efficiency = max(threads_speedup) / threads
            print(f"  Эффективность: {efficiency:.2%} от идеального ускорения")

def main():
    # Читаем результаты
    print("Чтение результатов из compare-blocks.txt...")
    results = read_results('compare-blocks.txt')

    if not results:
        print("Ошибка: не удалось прочитать результаты")
        return

    print(f"Прочитано {len(results)} записей")

    # Создаем графики зависимости от размера блока
    print("\nСоздание графиков зависимости от размера блока...")
    create_block_size_plots(results)

    # Создаем сравнительный график
    print("\nСоздание сравнительного графика...")
    create_block_size_comparison(results)

    # Анализ оптимальных размеров блоков
    analyze_optimal_blocks(results)

    print("\nГотово! Созданы файлы:")
    print("- block_size_matrix_100_threads_4.png")
    print("- block_size_matrix_200_threads_8.png")
    print("- block_size_matrix_500_threads_8.png")
    print("- block_size_comparison.png")

if __name__ == "__main__":
    main()