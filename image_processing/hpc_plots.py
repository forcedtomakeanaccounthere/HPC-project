import matplotlib.pyplot as plt
import numpy as np

# ----------------------------------------------------
# 1) Raw experimental data (from your console output)
# ----------------------------------------------------

granularity = ['small', 'medium', 'fine']   # labels for x-axis
x = np.arange(len(granularity))

# Sequential times (seconds)
# RESULT,seq,small,1,0.841000
# RESULT,seq,medium,1,3.194000
# RESULT,seq,fine,1,11.928000
T_seq = [
    0.841,   # small  (512x512)
    3.194,   # medium (1024x1024)
    11.928   # fine   (2048x2048)
]

# Parallel times with 6 threads
# RESULT,par,small,6,0.378000
# RESULT,par,medium,6,1.369000
# RESULT,par,fine,6,5.106000
T_par_6 = [
    0.378,   # small
    1.369,   # medium
    5.106    # fine
]

# Parallel times with 8 threads
# RESULT,par,small,8,0.360000
# RESULT,par,medium,8,1.338000
# RESULT,par,fine,8,4.938000
T_par_8 = [
    0.360,   # small
    1.338,   # medium
    4.938    # fine
]

p6 = 6
p8 = 8

# ----------------------------------------------------
# 2) Derived metrics: speedup, efficiency, cost
# ----------------------------------------------------

# Speedup S = T_seq / T_par
S_6 = [T_seq[i] / T_par_6[i] for i in range(len(granularity))]
S_8 = [T_seq[i] / T_par_8[i] for i in range(len(granularity))]

# Efficiency E = S / p
E_6 = [S_6[i] / p6 for i in range(len(granularity))]
E_8 = [S_8[i] / p8 for i in range(len(granularity))]

# Cost C = p * T_par  (total thread-seconds)
C_6 = [p6 * T_par_6[i] for i in range(len(granularity))]
C_8 = [p8 * T_par_8[i] for i in range(len(granularity))]

# Optional: print a small table in the console
print("Granularity | T_seq  | T_par_6 | T_par_8 | S_6  | S_8  | E_6  | E_8")
for i, g in enumerate(granularity):
    print(f"{g:10s} | {T_seq[i]:6.3f} | {T_par_6[i]:7.3f} | {T_par_8[i]:7.3f} | "
          f"{S_6[i]:4.2f} | {S_8[i]:4.2f} | {E_6[i]:4.2f} | {E_8[i]:4.2f}")

# ----------------------------------------------------
# 3) Plot 1 – Sequential time vs granularity
# ----------------------------------------------------

plt.figure()
plt.plot(granularity, T_seq, marker='o')
plt.xlabel('Granularity (problem size)')
plt.ylabel('Time (s)')
plt.title('Sequential Time vs Granularity')
plt.grid(True)
plt.tight_layout()
plt.savefig('1_seq_time_vs_granularity.png')

# ----------------------------------------------------
# 4) Plot 2 – Parallel time vs granularity (6 & 8 threads)
# ----------------------------------------------------

plt.figure()
plt.plot(granularity, T_par_6, marker='o', label='6 threads')
plt.plot(granularity, T_par_8, marker='s', label='8 threads')
plt.xlabel('Granularity (problem size)')
plt.ylabel('Time (s)')
plt.title('Parallel Time vs Granularity')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('2_par_time_vs_granularity.png')

# ----------------------------------------------------
# 5) Plot 3 – Speedup vs granularity
# ----------------------------------------------------

plt.figure()
plt.plot(granularity, S_6, marker='o', label='6 threads')
plt.plot(granularity, S_8, marker='s', label='8 threads')
plt.xlabel('Granularity (problem size)')
plt.ylabel('Speedup (T_seq / T_par)')
plt.title('Speedup vs Granularity')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('3_speedup_vs_granularity.png')

# ----------------------------------------------------
# 6) Plot 4 – Efficiency vs granularity
# ----------------------------------------------------

plt.figure()
plt.plot(granularity, E_6, marker='o', label='6 threads')
plt.plot(granularity, E_8, marker='s', label='8 threads')
plt.xlabel('Granularity (problem size)')
plt.ylabel('Efficiency')
plt.title('Efficiency vs Granularity')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('4_efficiency_vs_granularity.png')

# ----------------------------------------------------
# 7) Plot 5 – Cost vs granularity
# ----------------------------------------------------

plt.figure()
plt.plot(granularity, C_6, marker='o', label='6 threads')
plt.plot(granularity, C_8, marker='s', label='8 threads')
plt.xlabel('Granularity (problem size)')
plt.ylabel('Cost = p × T_par  (thread-seconds)')
plt.title('Cost vs Granularity')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('5_cost_vs_granularity.png')

print("\nAll 5 graphs saved as PNG files in this folder.")
