# esp32-c6_tft_pc_status_display

# ESP32-C6 PC System Status Monitor

A real-time PC status monitoring system using an **ESP32-C6 Super Mini** and an **ST7735 128x160 SPI Display**. The project consists of two main components: a Python-based server running on the PC that streams resource metrics (CPU, GPU, RAM, Disk, and Network DL/UL speeds), and C++ code running on the ESP32-C6 client that renders smooth, flicker-free graphics.

## 📸 Features

* **Real-Time Data Streaming:** Sends CPU, GPU, RAM, Disk, Download (DL), and Upload (UL) usages every second over TCP/IP sockets.

* **Flicker-Free Rendering:** Uses `GFXcanvas16` off-screen double buffering for smooth frame updates.

* **Dynamic Visual Gauges:** Features custom circular gauges for CPU, GPU, RAM, and Disk utilization with smooth color gradients (Green $\rightarrow$ Amber $\rightarrow$ Red).

* **Network Progress Bars:** Animated horizontal progress bars for upload and download rates.

* **Visual Status Indicators:** Pulse and blinking connection status indicators on the display.

* **Auto-Reconnect Handling:** Reconnects automatically if the Wi-Fi or socket connection drops on either side.

## 🛠️ System Architecture

```
┌────────────────────────────────┐         Wi-Fi (TCP Socket)        ┌──────────────────────────────────┐
│           PC Server            │ ────────────────────────────────> │      ESP32-C6 Super Mini         │
│  (Python + psutil + nvidia-smi)│         Port 5001 (Data)          │  (ST7735 Display / Adafruit GFX) │
└────────────────────────────────┘                                   └──────────────────────────────────┘

```

## 📁 Hardware Requirements

1. **ESP32-C6 Super Mini**

2. **ST7735 TFT LCD Display** (128x160 Resolution, SPI Interface)

3. Connecting Wires & Power Source (USB-C)

### Pin Configuration (ESP32-C6 to ST7735)

| ESP32-C6 Pin | ST7735 Display Pin | Description | 
 | ----- | ----- | ----- | 
| **GPIO 14** | `CS` | Chip Select | 
| **GPIO 15** | `DC` | Data / Command | 
| **GPIO 18** | `RST` | Reset | 
| **GPIO 8** | `BLK` / `LED` | Backlight Control | 
| **GPIO 20** | `SDA` / `MOSI` | SPI Data | 
| **GPIO 19** | `SCL` / `SCK` | SPI Clock | 
| **3.3V / 5V** | `VCC` | Power Supply | 
| **GND** | `GND` | Ground | 

## 💻 Software Prerequisites

### 1. PC Server (Python)

* Python 3.x

* Required Python packages:

  ```
  pip install psutil
  
  ```

* *(Optional)* NVIDIA GPU with `nvidia-smi` drivers installed for GPU utilization monitoring.

### 2. Microcontroller Client (ESP32)

* Arduino IDE or PlatformIO

* Board Package: ESP32 by Espressif

* Required Arduino Libraries:

  * `Adafruit_GFX`

  * `Adafruit_ST7735`

  * `SPI`

  * `WiFi`

## 🚀 Setup & Installation

### Step 1: Configure and Run the PC Server

1. Open `pc_server.py` in your editor.

2. Adjust `max_speed_mb` if necessary to match your internet bandwidth limit in MB/s (default is set to `6.25` MB/s $\approx$ 50 Mbps):

   ```
   max_speed_mb = 6.25
   
   ```

3. Run the script on your host machine:

   ```
   python pc_server.py
   
   ```

4. Note down your PC's IP address (e.g., `192.168.1.116`).

### Step 2: Configure and Flash the ESP32-C6

1. Open the ESP32 code in your IDE.

2. Update the Wi-Fi credentials and PC Server IP address in the configuration section:

   ```
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* pcIP = "192.168.1.XXX"; // Your PC's Local IP Address
   const uint16_t port = 5001;
   
   ```

3. Connect the ESP32-C6 via USB, select the target port, and upload the code.

## 📡 Protocol & Data Format

The PC transmits plain text lines over TCP socket formatted as follows:

```
CPU:<cpu_val>,GPU:<gpu_val>,RAM:<ram_val>,DSK:<dsk_val>,DL:<dl_val>%,UL:<ul_val>%

```

**Example Data String:**

```
CPU:24,GPU:12,RAM:48,DSK:65,DL:15.20%,UL:2.10%

```

## ⚙️ Customization

* **Gradients & Colors:** Modify RGB definitions in the C++ file to change color schemes.

* **Network Maximums:** Adjust `max_speed_mb` in `pc_server.py` to scale download and upload percentage bars accurately based on your local network speed.

* **FPS & Smoothness:** Tweak `FRAME_MS` or the `ease()` function factor on the ESP32 to change smooth transition speeds.

## 📜 License

This project is licensed under the [GNU General Public License v3.0 (GPL-3.0)](https://www.gnu.org/licenses/gpl-3.0.html).
