# VelociTrade Mini — Low-Latency Trading Lab (C++/Linux)

A compact trading lab that demonstrates the core building blocks of low-latency systems:

- **Market Data (UDP):** exchange publishes top-of-book snapshots at ~10 kHz  
- **Order Path (TCP + Framing):** length-prefixed messages, heartbeats, reconnects  
- **Order Book:** minimal L2 structure with atomic snapshot updates  
- **Strategy:** queue imbalance → join best bid/ask  
- **Risk Controls:** price band, notional cap, per-second rate limit  
- **Latency Measurement:** per-order round-trip times in nanoseconds, logged to a binary file and visualized with Python  

---

## 🔧 Quick Start

### ✅ Build
```bash
mkdir -p build && cd build
cmake .. && cmake --build . -j
```

### 🚀 Run
In two terminals:

**Terminal A (exchange simulator):**
```bash
./ex_sim ../config.yaml
```

**Terminal B (client/strategy):**
```bash
./uclient ../config.yaml 30000
```

This will simulate sending 30,000 orders at high speed.

---

### 📈 Plot Latency
```bash
python3 ../tools/plot_latency.py ../latency.bin
```

This generates a histogram and saves it as `latency.png`.


## 📁 Project Structure
```
.
├── CMakeLists.txt
├── config.yaml
├── include/
│   ├── core/        # timing, logging, framing, config utils
│   ├── proto/       # packed wire structs
│   ├── book/        # minimal L2 order book
│   └── net/         # UDP networking helpers
├── src/
│   ├── sim/ex_sim.cpp      # exchange simulator (market data + TCP order handling)
│   └── client/uclient.cpp  # trading client: book → strategy → risk → orders
└── tools/
    └── plot_latency.py     # binary latency log → matplotlib histogram
```


## ⚙️ Configuration (`config.yaml`)
```yaml
md_ip: 127.0.0.1
md_port: 5001
ord_ip: 127.0.0.1
ord_port: 9001
levels: 8
risk_band_ticks: 20
risk_notional_cap: 1000000000
risk_max_rate: 2000
heartbeat_ms: 200
reconnect_ms: 500
```

## 📦 Requirements

- **C++20** compiler (GCC ≥ 11 recommended)
- **CMake ≥ 3.16**
- **Python 3** with `matplotlib` (for latency plots)

### Ubuntu / WSL Install
```bash
sudo apt update
sudo apt install build-essential cmake python3-matplotlib -y
```