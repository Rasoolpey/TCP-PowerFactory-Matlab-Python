# dynamic_tcp_server.py – Multi-component Server (Multiprocess)

import socket
import struct
import importlib
import multiprocessing

HOST = '127.0.0.1'

# Define configuration for each component
COMPONENTS = [
    {"port": 9101, "num_inputs": 7, "num_outputs": 2, "module": "logic_module1"},
    {"port": 9102, "num_inputs": 7, "num_outputs": 2, "module": "logic_module2"},
    {"port": 9103, "num_inputs": 7, "num_outputs": 2, "module": "logic_module3"},
]

def handle_client(conn, addr, cfg, logic_fn):
    port = cfg["port"]
    num_inputs = cfg["num_inputs"]
    num_outputs = cfg["num_outputs"]

    print(f"[TCP {port}] Connected to {addr}")
    conn.settimeout(1.0)

    try:
        data = conn.recv(num_inputs * 8)
        if not data or len(data) < num_inputs * 8:
            print(f"[TCP {port}] Incomplete or no data received.")
            return

        inputs = list(struct.unpack(f'{num_inputs}d', data))
        print(f"[TCP {port}] Received: {inputs}")

        outputs = logic_fn(inputs)
        if len(outputs) != num_outputs:
            print(f"[TCP {port}] Warning: Output mismatch!")
            outputs = [0.0] * num_outputs

        conn.sendall(struct.pack(f'{num_outputs}d', *outputs))
        print(f"[TCP {port}] Sent: {outputs}")

    except socket.timeout:
        print(f"[TCP {port}] Timeout")
    except Exception as e:
        print(f"[TCP {port}] Error: {e}")
    finally:
        conn.close()

def start_server(cfg):
    port = cfg["port"]
    module_name = cfg["module"]

    try:
        logic_module = importlib.import_module(module_name)
        logic_fn = getattr(logic_module, "compute_outputs")
    except Exception as e:
        print(f"[TCP {port}] Failed to import {module_name}. Error: {e}")
        return

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, port))
        s.listen(1)
        print(f"[TCP {port}] Listening with logic: {module_name}.compute_outputs")

        while True:
            try:
                conn, addr = s.accept()
                handle_client(conn, addr, cfg, logic_fn)
            except KeyboardInterrupt:
                print(f"[TCP {port}] Shutting down...")
                break

if __name__ == '__main__':
    processes = []
    for cfg in COMPONENTS:
        p = multiprocessing.Process(target=start_server, args=(cfg,))
        p.start()
        processes.append(p)

    print("[TCP Server] All component servers started (multiprocess). Press Ctrl+C to stop.")

    try:
        for p in processes:
            p.join()
    except KeyboardInterrupt:
        print("[TCP Server] Shutting down all components.")
        for p in processes:
            p.terminate()
