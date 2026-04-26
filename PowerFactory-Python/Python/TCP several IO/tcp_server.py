# tcp_server.py

import socket
import struct
from logic_module import compute_outputs  # Your custom logic here

HOST = '127.0.0.1'
PORT = 9090

NUM_INPUTS = 4    # <- change here if inputs change
NUM_OUTPUTS = 2   # <- change here if outputs change

def handle_client(conn, addr):
    print(f"[TCP Server] Connected to {addr}")
    conn.settimeout(1.0)

    try:
        data = conn.recv(NUM_INPUTS * 8)
        if not data or len(data) < NUM_INPUTS * 8:
            print("[TCP Server] Incomplete or no data received.")
            return

        # Unpack all inputs as doubles
        inputs = list(struct.unpack(f'{NUM_INPUTS}d', data))
        print(f"[TCP Server] Received: {inputs}")

        # Compute outputs from logic module
        outputs = compute_outputs(inputs)

        if len(outputs) != NUM_OUTPUTS:
            print("[TCP Server] Error: logic_module returned wrong number of outputs!")
            outputs = [0.0] * NUM_OUTPUTS

        # Pack and send outputs
        conn.sendall(struct.pack(f'{NUM_OUTPUTS}d', *outputs))
        print(f"[TCP Server] Sent: {outputs}")

    except socket.timeout:
        print("[TCP Server] Timeout - no data for 1 second.")
    except Exception as e:
        print(f"[TCP Server] Error: {e}")
    finally:
        conn.close()

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen(1)
        print(f"[TCP Server] Listening on {HOST}:{PORT}...")

        while True:
            try:
                conn, addr = s.accept()
                handle_client(conn, addr)
            except KeyboardInterrupt:
                print("[TCP Server] Shutting down...")
                break

if __name__ == '__main__':
    start_server()
