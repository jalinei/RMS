import pandas as pd 
import matplotlib.pyplot as plt

fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(10, 6))
file = pd.read_csv('timing_test')
# Create a mask to filter out rows whe
# re time1 is 0
mask = file['time1'] != 0
ax2.step(file["time"], file["level"], where="post", label='RMS_0')
ax1.step(file["time1"][mask], file["level1"][mask], where="post", color="orange", label='RMS_1')
# Add labels, title, and legend if necessary
ax2.set_ylabel('RMS_O')
ax2.legend()
ax1.set_ylabel('RMS_1')
ax1.legend()
ax2.set_xlabel('Time')

# Show the plot
plt.tight_layout()
plt.show()