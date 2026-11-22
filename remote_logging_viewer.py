import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5555))
print("Listening for ESP32 logs...")
while True:
    data, addr = sock.recvfrom(1024)
    print(data.decode('utf-8'), end='')