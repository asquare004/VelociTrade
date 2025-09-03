import struct, sys
import matplotlib.pyplot as plt

path = sys.argv[1] if len(sys.argv)>1 else '../latency.bin'
vals = []
with open(path,'rb') as f:
    while chunk := f.read(8):
        vals.append(struct.unpack('<q', chunk)[0])
vals.sort()
import numpy as np
p = lambda q: vals[int(q*(len(vals)-1))]
print(f"count={len(vals)} p50={p(0.5)} ns p99={p(0.99)} ns p99.9={p(0.999)} ns")
plt.hist(vals, bins=60)
plt.title('Order→Ack RTT (ns)')
plt.xlabel('ns'); plt.ylabel('count')
plt.savefig("latency.png")
print("Plot saved as latency.png")