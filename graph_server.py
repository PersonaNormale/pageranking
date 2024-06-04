import socket
import threading
import tempfile
import subprocess
import os
import logging
import signal
import sys

# Configurazione del logging
logging.basicConfig(filename='server.log', level=logging.INFO,
                    format='%(asctime)s %(message)s')

# Numero di matricola
port = 54348

# Lista di thread attivi
threads = []


def handle_client(client_socket):
    temp_filename = None
    try:
        # Leggere numero di nodi e archi
        n = int.from_bytes(client_socket.recv(4), byteorder='little')
        a = int.from_bytes(client_socket.recv(4), byteorder='little')

        logging.info(f"Received graph with {n} nodes and {a} arcs")

        # Creare un file temporaneo
        with tempfile.NamedTemporaryFile(delete=False, mode='w') as temp_file:
            temp_filename = temp_file.name

            # Scrivere il numero di nodi e archi nel file
            temp_file.write(
                f"%%MatrixMarket matrix coordinate pattern general\n")
            temp_file.write(f"{n} {n} {a}\n")

            # Iniziare a leggere gli archi
            valid_arcs = 0
            invalid_arcs = 0
            while valid_arcs + invalid_arcs < a:
                for _ in range(min(10, a - valid_arcs - invalid_arcs)):
                    orig = int.from_bytes(
                        client_socket.recv(4), byteorder='little')
                    dest = int.from_bytes(
                        client_socket.recv(4), byteorder='little')
                    if 1 <= orig <= n and 1 <= dest <= n:
                        temp_file.write(f"{orig} {dest}\n")
                        valid_arcs += 1
                    else:
                        invalid_arcs += 1

        logging.info(
            f"Writing to temporary file {temp_filename}: {valid_arcs} valid arcs, {invalid_arcs} invalid arcs")

        # Eseguire pagerank
        pagerank_path = './pagerank'  # Usa il percorso relativo
        result = subprocess.run(
            [pagerank_path, temp_filename], capture_output=True, text=True)
        logging.info(f"pagerank executed with exit code {result.returncode}")

        if result.returncode == 0:
            output = result.stdout
            client_socket.sendall((0).to_bytes(
                4, byteorder='little') + output.encode())
        else:
            error_output = result.stderr
            client_socket.sendall(result.returncode.to_bytes(
                4, byteorder='little') + error_output.encode())

        # Log dei dati
        logging.info(
            f"Graph with {n} nodes, file {temp_filename}, {invalid_arcs} invalid arcs, {valid_arcs} valid arcs, pagerank exit code {result.returncode}")

    except Exception as e:
        logging.error(f"Error handling client: {e}")
    finally:
        client_socket.close()
        if temp_filename and os.path.exists(temp_filename):
            os.remove(temp_filename)


def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind(('127.0.0.1', port))
    server.listen(5)
    logging.info(f"Server started on port {port}")

    def signal_handler(sig, frame):
        for t in threads:
            t.join()
        logging.info("Server shutdown")
        print("Bye dal server")
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    while True:
        client_socket, addr = server.accept()
        logging.info(f"Accepted connection from {addr}")
        client_handler = threading.Thread(
            target=handle_client, args=(client_socket,))
        client_handler.start()
        threads.append(client_handler)


if __name__ == "__main__":
    start_server()
