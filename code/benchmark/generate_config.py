import os

def generate_config_files(output_folder, datasets, algorithms, window_slide_pairs, query_label_pairs, z_scores, watermarks):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    for dataset in datasets:
        for algorithm in algorithms:
            for size, slide in window_slide_pairs:
                for query_type, labels in query_label_pairs:
                    for zscore in z_scores:
                        for watermark, ooo_strategy in watermarks:
                            config_content = (
                                f"algorithm={algorithm}\n"
                                f"input_data_path={dataset}\n"
                                f"output_base_folder={output_folder}\n"
                                f"size={size}\n"
                                f"slide={slide}\n"
                                f"query_type={query_type}\n"
                                f"labels={','.join(map(str, labels))}\n"
                                f"zscore={zscore}\n"
                                f"watermark={watermark}\n"
                                f"ooo_strategy={ooo_strategy}\n"
                            )
                            config_filename = f"config_a{algorithm}_S{size}_s{slide}_q{query_type}_z{zscore}_wm{watermark}.txt"
                            config_filepath = os.path.join(output_folder, config_filename)
                            with open(config_filepath, 'w') as config_file:
                                config_file.write(config_content)
                            print(f"Generated {config_filepath}")

def main():
    output_folder = "code/benchmark/results_so/"
    datasets = ["code/dataset/so/so-stream_labelled.txt"]
    algorithms = [3]
    window_slide_pairs = [(86400, 43200), (129600, 43200), (172800, 86400), (259200, 86400)]
    query_label_pairs = [(1,[1]), (4, [1, 2, 3]), (3, [1, 2]),  (2, [1, 2]),  (5, [3, 1, 2]), (6, [2, 1, 3]), (7, [1, 2, 3, 1]) , (8, [1, 2]), (9, [3, 1, 2]), (10, [1, 2, 3])]
    z_scores = [0.65, 1.96]
    watermarks = [(0, 1)]

    generate_config_files(output_folder, datasets, algorithms, window_slide_pairs, query_label_pairs, z_scores, watermarks)

if __name__ == "__main__":
    main()