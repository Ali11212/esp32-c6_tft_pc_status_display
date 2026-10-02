import socket
import psutil
import time
import subprocess

HOST = '0.0.0.0'  # tüm IP’lerden dinle
PORT = 5001

# Ağ hızlarını başlat
net_io_prev = psutil.net_io_counters()

# Socket oluştur ve portu tekrar kullanılabilir yap
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((HOST, PORT))
s.listen(1)
print("PC Server çalışıyor, ESP32 bekleniyor...")

conn, addr = s.accept()
print("ESP32 bağlandı:", addr)

def get_gpu_usage():
    try:
        result = subprocess.check_output([
            "nvidia-smi",
            "--query-gpu=utilization.gpu",
            "--format=csv,noheader,nounits"
        ])
        return int(result.decode().strip())
    except Exception:
        return 0  # GPU yoksa veya hata olursa 0%

while True:
    # CPU yüzdesi
    cpu = int(psutil.cpu_percent(interval=1))

    # GPU yüzdesi
    gpu = get_gpu_usage()

    # RAM yüzdesi
    ram = int(psutil.virtual_memory().percent)

    # Disk kullanım yüzdesi
    dsk = int(psutil.disk_usage("/").percent)

    # Ağ verisi (DL/UL)
    # Ağ verisi (DL/UL)
    net_io = psutil.net_io_counters()
    dl_mb = (net_io.bytes_recv - net_io_prev.bytes_recv) / (1024 * 1024)  # MB
    ul_mb = (net_io.bytes_sent - net_io_prev.bytes_sent) / (1024 * 1024)  # MB
    net_io_prev = net_io

    # Maksimum hız tahmini (MB/s), buna göre yüzdelik hesapla
    max_speed_mb = 6.25  # internet hızına göre ayarla
    dl = round((dl_mb / max_speed_mb) * 100, 2)
    ul = round((ul_mb / max_speed_mb) * 100, 2)

    # Hareketli ortalama için (stabil gösterim)
    alpha = 0.7
    if 'dl_avg' not in globals(): dl_avg = dl
    if 'ul_avg' not in globals(): ul_avg = ul
    dl_avg = alpha * dl + (1 - alpha) * dl_avg
    ul_avg = alpha * ul + (1 - alpha) * ul_avg

    line = f"CPU:{cpu},GPU:{gpu},RAM:{ram},DSK:{dsk},DL:{dl_avg:.2f}%,UL:{ul_avg:.2f}%\n"

    # Gönderirken hata yakala
    try:
        conn.sendall(line.encode())
        print(f"DL:{dl_avg:.2f}%, UL:{ul_avg:.2f}%")
    except (BrokenPipeError, ConnectionResetError):
        print("ESP32 bağlantısı koptu. Bekleniyor...")
        conn, addr = s.accept()
        print("ESP32 tekrar bağlandı:", addr)

    time.sleep(1)