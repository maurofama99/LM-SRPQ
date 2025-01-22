import networkx as nx
import matplotlib.pyplot as plt

# Funzione per leggere la lista di adiacenza da un file
def read_adj_list(file_path):
    adj_list = {}
    with open(file_path, 'r') as f:
        for line in f:
            # Rimuovi eventuali spazi extra e newline
            line = line.strip()
            # Se la linea non è vuota
            if line:
                # Dividi il nodo dalla lista dei vicini
                node, neighbors = line.split(':')
                # Rimuovi eventuali spazi e convertili in una lista
                neighbors = neighbors.strip()[1:-1].split(', ') if neighbors.strip() else []
                adj_list[node] = neighbors
    return adj_list

# Funzione principale
def main():
    # Percorso del file di input
    file_path = 'graph_aj.txt'  # Sostituisci con il percorso del tuo file

    # Lettura della lista di adiacenza dal file
    adj_list = read_adj_list(file_path)

    # Creazione del grafo da lista di adiacenza
    G = nx.Graph(adj_list)

    # Disegno del grafo
    plt.figure(figsize=(8, 8))
    nx.draw(G, with_labels=True, node_color='lightblue', node_size=3000, font_size=10, font_weight='bold', edge_color='gray')
    plt.title("Grafo dalla lista di adiacenza")
    plt.show()

# Punto di ingresso del programma
if __name__ == "__main__":
    main()
