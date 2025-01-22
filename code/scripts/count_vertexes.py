def count_unique_vertices(file_path):
    unique_vertices = set()

    # Legge il file e processa ogni riga
    with open(file_path, 'r') as file:
        for line in file:
            if line.strip():  # Salta righe vuote
                vertice1, vertice2, *_ = line.split()
                unique_vertices.add(int(vertice1))
                unique_vertices.add(int(vertice2))

    return len(unique_vertices)

# Esempio di utilizzo
if __name__ == "__main__":
    file_path = "/Users/maurofama/Documents/phd/frames4pgs/LM-SRPQ/code/dataset/so-stream_postprocess_debug.txt"  # Sostituisci con il tuo file
    num_unique_vertices = count_unique_vertices(file_path)
    print(f"Numero di vertici univoci: {num_unique_vertices}")
