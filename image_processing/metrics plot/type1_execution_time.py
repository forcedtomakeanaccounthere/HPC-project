import matplotlib.pyplot as plt
import numpy as np

plt.style.use("seaborn-v0_8")

# ------------------------------------------------------------
# Raw Data
# ------------------------------------------------------------

methods = ["Sequential", "Parallel", "Hybrid"]
x = np.arange(len(methods))

# Sequential times
T_seq_small  = 0.8420
T_seq_medium = 3.3020
T_seq_fine   = 12.1860

# ---------- 1) Medium image: varying threads ----------
par_medium_by_threads = {3: 2.0530, 6: 1.9570, 8: 1.9360, 12: 1.9330, 15: 1.9260}
hyb_medium_by_threads = {3: 1.2751, 6: 1.2445, 8: 1.3129, 12: 1.2633, 15: 1.3226}

# ---------- 2) Fine image: processors = threads ----------
par_fine_by_procs = {2: 8.7610, 5: 6.9300, 9: 6.8020}
hyb_fine_by_procs = {2: 4.2071, 5: 5.4811, 9: 4.4037}

# ---------- 3) Granularity (small / medium / fine) ----------
T_par_small_8  = 0.3600
T_par_medium_8 = 1.3380
T_par_fine_8   = 4.9380

T_hyb_small_8  = 0.4693
T_hyb_medium_8 = 1.2885
T_hyb_fine_8   = 4.6009

granularity_data = {
    "Small":  (T_seq_small,  T_par_small_8,  T_hyb_small_8),
    "Medium": (T_seq_medium, T_par_medium_8, T_hyb_medium_8),
    "Fine":   (T_seq_fine,   T_par_fine_8,   T_hyb_fine_8),
}

# ------------------------------------------------------------
# Plot Figure with 3 Neat Subplots
# ------------------------------------------------------------

fig, axs = plt.subplots(1, 3, figsize=(20, 6))
colors = ["#0077b6", "#ef476f", "#06d6a0", "#ffd166", "#8a2be2"]

# ==================================================
# 1) Execution Time vs Method (Threads)
# ==================================================
for idx, threads in enumerate([3, 6, 8, 12, 15]):
    seq = T_seq_medium
    par = par_medium_by_threads[threads]
    hyb = hyb_medium_by_threads[threads]
    axs[0].plot(x, [seq, par, hyb], marker="o", linewidth=2,
                color=colors[idx], label=f"{threads} Threads")

axs[0].set_title("Execution Time vs Method\n(Varying Threads, Medium Image)", fontsize=14)
axs[0].set_xticks(x)
axs[0].set_xticklabels(methods, fontsize=12, rotation=15)
axs[0].set_ylabel("Execution Time (seconds)", fontsize=12)
axs[0].grid(True, linestyle="--", alpha=0.6)
axs[0].legend(fontsize=10)

# ==================================================
# 2) Execution Time vs Method (Processors)
# ==================================================
for idx, procs in enumerate([2, 5, 9]):
    seq = T_seq_fine
    par = par_fine_by_procs[procs]
    hyb = hyb_fine_by_procs[procs]
    axs[1].plot(x, [seq, par, hyb], marker="o", linewidth=2,
                color=colors[idx], label=f"{procs} Processors")

axs[1].set_title("Execution Time vs Method\n(Varying Processors, Fine Image)", fontsize=14)
axs[1].set_xticks(x)
axs[1].set_xticklabels(methods, fontsize=12, rotation=15)
axs[1].set_ylabel("Execution Time (seconds)", fontsize=12)
axs[1].grid(True, linestyle="--", alpha=0.6)
axs[1].legend(fontsize=10)

# ==================================================
# 3) Execution Time vs Method (Granularity)
# ==================================================
for idx, (gran, values) in enumerate(granularity_data.items()):
    axs[2].plot(x, values, marker="o", linewidth=2,
                color=colors[idx], label=f"{gran} Image")

axs[2].set_title("Execution Time vs Method\n(Varying Granularity, 8 Threads)", fontsize=14)
axs[2].set_xticks(x)
axs[2].set_xticklabels(methods, fontsize=12, rotation=15)
axs[2].set_ylabel("Execution Time (seconds)", fontsize=12)
axs[2].grid(True, linestyle="--", alpha=0.6)
axs[2].legend(fontsize=10)

plt.tight_layout()
plt.show()
