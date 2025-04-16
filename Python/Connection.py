import socket
import threading
import time
import random

NUM_CUBES = 100
START_PORT = 5500
SEND_INTERVAL = .005  # seconds

class CubeClient(threading.Thread):
    def __init__(self, port, cube_id):
        super().__init__()
        self.port = port
        self.cube_id = cube_id
        self.sock = None
        self.running = True

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.sock.connect(("127.0.0.1", self.port))
            self.sock.settimeout(1.0)
            print(f"[Cube {self.cube_id}] Connected to port {self.port}")
        except Exception as e:
            print(f"[Cube {self.cube_id}] Connection failed: {e}")
            self.running = False

    def run(self):
        self.connect()
        while self.running:
            try:
                # Generate simple control input
                x = random.uniform(-1, 1)
                y = random.uniform(-1, 1)
                yaw = random.uniform(0.0, 0.0)  # No yaw control in unreal
                # Send control input to Unreal Engine

                msg = f"{x:.2f} {y:.2f} {yaw:.2f}\n"
                self.sock.sendall(msg.encode('utf-8'))

                # Try to read response from Unreal
                try:
                    response = self.sock.recv(1024)
                    if response:
                        print(f"[Cube {self.cube_id}] Response: {response.decode('utf-8').strip()}")
                    else:
                        print(f"[Cube {self.cube_id}] No response — Unreal may have dropped connection.")
                        self.running = False
                except socket.timeout:
                    pass  # expected if no reply quickly

                time.sleep(SEND_INTERVAL)

            except Exception as e:
                print(f"[Cube {self.cube_id}] Error: {e}")
                self.running = False

        if self.sock:
            self.sock.close()
            print(f"[Cube {self.cube_id}] Socket closed.")

# Launch all cube threads
clients = []

for i in range(NUM_CUBES):
    port = START_PORT + i
    client = CubeClient(port, i)
    client.start()
    clients.append(client)

try:
    while any(client.is_alive() for client in clients):
        time.sleep(1)

except KeyboardInterrupt:
    print("Stopping all clients...")
    for client in clients:
        client.running = False
    for client in clients:
        client.join()
