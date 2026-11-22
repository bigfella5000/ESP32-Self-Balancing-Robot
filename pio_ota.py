import sys, os, socket, threading, http.server, socketserver, time, shutil

if len(sys.argv) < 3:
    print("Usage: python pio_ota.py <firmware.bin> <esp32_ip>")
    sys.exit(1)

firmware_path = sys.argv[1]
esp32_ip = sys.argv[2]
port = 8000

# 1. Prepare directory and file
build_dir = os.path.dirname(firmware_path)
os.chdir(build_dir)

source_file = os.path.basename(firmware_path)
if source_file != "firmware.bin":
    shutil.copy(source_file, "firmware.bin")

# 2. Start background HTTP server
class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[HTTP] {self.client_address[0]} requested {args[0]}")

httpd = socketserver.TCPServer(("", port), QuietHandler)
threading.Thread(target=httpd.serve_forever, daemon=True).start()
print(f"\n[OTA] Local server hosting firmware.bin on port {port}...")

# 3. Send UDP trigger to ESP32
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(b"UPDATE", (esp32_ip, 4321))
print(f"[OTA] Sent UPDATE command to {esp32_ip}:4321")

# 4. Keep server alive while ESP32 downloads
print("[OTA] Waiting 15 seconds for ESP32 to download and flash...")
time.sleep(15)
print("[OTA] Push complete. Closing server.\n")