# Funzione per leggere un file e calcolare la somma dei numeri
import sys

def somma_numeri_da_file(percorso_file):
    try:
        with open(percorso_file, 'r') as file:
            righe = file.readlines()
            # Converti ogni riga in un numero e calcola la somma
            somma_totale = sum(int(riga.strip()) for riga in righe if riga.strip().isdigit())
        return somma_totale
    except FileNotFoundError:
        print(f"Errore: Il file '{percorso_file}' non esiste.")
        sys.exit(1)
    except ValueError:
        print("Errore: Il file contiene valori non numerici non validi.")
        sys.exit(1)

# Esempio di utilizzo
if __name__ == "__main__":
    # Definizione del percorso del file come variabile
    percorso_file = "/Users/maurofama/Documents/phd/frames4pgs/LM-SRPQ/code/LM-memory_1_1_2.txt"  # Sostituisci con il nome del file desiderato
    somma = somma_numeri_da_file(percorso_file)
    print(f"La somma totale dei numeri nel file \"{percorso_file}\" è: {somma}")