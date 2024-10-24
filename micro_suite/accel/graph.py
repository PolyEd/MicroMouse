import sys
import matplotlib.pyplot as plt
fig, (ax0, ax1, ax2) = plt.subplots(3, 1)
lines = [line.split(',') for line in sys.stdin]
jump = int(len(lines) / 1000)
for i in range(0, 1000):
    ax0.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][1]), c=['#ff0000'], s=1)
    ax0.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][2]), c=['#00ff00'], s=1)
    ax0.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][3]), c=['#0000ff'], s=1)
    ax1.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][4]), c=['#ff0000'], s=1)
    ax1.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][5]), c=['#00ff00'], s=1)
    ax1.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][6]), c=['#0000ff'], s=1)
    ax2.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][7]), c=['#ff0000'], s=1)
    ax2.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][8]), c=['#00ff00'], s=1)
    ax2.scatter(float(lines[i * jump][0]),
                float(lines[i * jump][9]), c=['#0000ff'], s=1)
plt.show()
