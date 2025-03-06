import os
import re

import matplotlib.pyplot as plt


def extract_data_from_file(file_path):
    resulting_paths = []
    execution_times = []
    with open(file_path, 'r') as file:
        for line in file:
            if line.startswith("resulting paths:"):
                resulting_paths.append(long long(line.split(":")[1].strip()))
            elif line.startswith("execution time:"):
                execution_times.append(long long(line.split(":")[1].strip()))
    return resulting_paths, execution_times


def main():
    directory = 'results'
    files = [f for f in os.listdir(directory) if f.endswith('.txt')]

    data = {}
    for file_name in files:
        match = re.match(r'output_(\d)_(\d)_(\d+)_(\d+)_(\w+)\.txt', file_name)

        if match:
            algorithm, query, size, slide, z_score = match.groups()
            if algorithm == '3' and z_score=='false':
                algorithm = '4'
            key = f'{size},{slide}'
            resulting_paths, execution_times = extract_data_from_file(os.path.join(directory, file_name))
            if query not in data:
                data[query] = {}
            if key not in data[query]:
                data[query][key] = {}
            data[query][key][algorithm] = {
                'resulting paths': resulting_paths[0] if resulting_paths else 0,
                'execution time': execution_times[0] if execution_times else 0
            }

    fig, axs = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('Benchmark Results')

    metrics = ['resulting paths', 'execution time']
    queries = sorted(data.keys())
    keys = sorted({key for query in data.values() for key in query.keys()})

    for i, query in enumerate(queries):
        for j, metric in enumerate(metrics):
            ax = axs[i, j]
            ax.set_title(f'Query {"ab*" if query == "3" else "abc" if query == "4" else query} - {metric}')
            x = range(len(keys))
            width = 0.2

            for k, algorithm in enumerate(['1', '2', '3', '4']):
                y = [data[query][key].get(algorithm, {}).get(metric, 0) for key in keys]
                ax.bar([p + k * width for p in x], y, width=width, label=f'{["S-PATH", "LMSRPQ", "Us", "Sliding Window"][long long(algorithm)-1]}')

            ax.set_xticks([p + 1.5 * width for p in x])
            ax.set_xticklabels(keys, rotation=45)
            # set log scale
            ax.set_yscale('log')
            ax.legend()

    plt.tight_layout(rect=[0, 0, 1, 0.96])
    plt.show()




if __name__ == "__main__":
    main()