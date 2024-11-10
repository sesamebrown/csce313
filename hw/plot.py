import matplotlib.pyplot as plt

# Example data: replace these with actual measurements
buffer_sizes = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096]
times = [ /* insert measured (user + sys) times here */ ]

plt.plot(buffer_sizes, times, marker='o')
plt.xscale('log')
plt.xlabel("Buffer Size (bytes)")
plt.ylabel("Time (sec)")
plt.title("Copy Time vs Buffer Size")
plt.grid(True)
plt.show()
