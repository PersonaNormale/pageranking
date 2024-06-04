import socket
import threading
import sys
import logging

# Configurazione del logging
logging.basicConfig(filename='client.log', level=logging.INFO,
                    format='%(asctime)s %(message)s')

# Numero di matricola
port = 54348


def handle_file(filename):
    try:
        with open(filename, 'r') as file:
            lines = file.readlines()

            # Saltare le linee di commento
            data_lines = [line for line in lines if not line.startswith('%')]

            # Leggere il numero di nodi e archi
            n, m, a = map(int, data_lines[0].split())
            edges = data_lines[1:]

            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect(('127.0.0.1', port))

                # Inviare numero di nodi e archi
                s.sendall(n.to_bytes(4, byteorder='little'))
                s.sendall(a.to_bytes(4, byteorder='little'))

                # Inviare gli archi
                for edge in edges:
                    parts = edge.split()
                    if len(parts) == 2:
                        orig, dest = map(int, parts)
                        s.sendall(orig.to_bytes(4, byteorder='little'))
                        s.sendall(dest.to_bytes(4, byteorder='little'))
                    else:
                        logging.warning(
                            f"Invalid edge in file {filename}: {edge}")

                # Ricevere la risposta dal server
                exit_code = int.from_bytes(s.recv(4), byteorder='little')
                response = s.recv(4096).decode()

                # Stampare il risultato
                print(f"{filename} Exit code: {exit_code}")
                print(response)
                print(f"{filename} Bye")

    except Exception as e:
        logging.error(f"Error handling file {filename}: {e}")


if __name__ == "__main__":
    files = sys.argv[1:]

    threads = []
    for filename in files:
        thread = threading.Thread(target=handle_file, args=(filename,))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()
