import os
import numpy as np
import matplotlib.pyplot as plt

# ==========================
# Make output folder
# ==========================
OUTPUT_DIR = "output images"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# ==========================
# Raw timing data (seconds)
# ==========================

# Image sizes (pixels)
PIX_SMALL = 512 * 512
PIX_MED   = 1024 * 1024
PIX_FINE  = 2048 * 2048

# Sequential (single-thread) times
T_seq_small  = 0.8420
T_seq_medium = 3.3020
T_seq_fine   = 12.1860

# Parallel – medium image, varying threads
par_medium_by_threads = {
    3: 2.0530,
    6: 1.9570,
    8: 1.9360,
    12: 1.9330,
    15: 1.9260,
}

# Hybrid (GPU) – medium image, varying threads
hyb_medium_by_threads = {
    3: 1.2751,
    6: 1.2445,
    8: 1.3129,
    12: 1.2633,
    15: 1.3226,
}

# Parallel – fine image, "processors" = threads {2,5,9}
par_fine_by_procs = {
    2: 8.7610,
    5: 6.9300,
    9: 6.8020,
}

# Hybrid – fine image, processors {2,5,9}
hyb_fine_by_procs = {
    2: 4.2071,
    5: 5.4811,
    9: 4.4037,
}

# Granularity view (8 threads)
# Parallel
T_par_small_8  = 0.3600
T_par_medium_8 = 1.3380
T_par_fine_8   = 4.9380

# Hybrid
T_hyb_small_8  = 0.4693
T_hyb_medium_8 = 1.2885
T_hyb_fine_8   = 4.6009

granularity_data_time = {
    "Small":  (T_seq_small,  T_par_small_8,  T_hyb_small_8,  PIX_SMALL),
    "Medium": (T_seq_medium, T_par_medium_8, T_hyb_medium_8, PIX_MED),
    "Fine":   (T_seq_fine,   T_par_fine_8,   T_hyb_fine_8,   PIX_FINE),
}

# Colors for lines (Sequential, Parallel, Hybrid)
COLOR_SEQ = "#1f77b4"
COLOR_PAR = "#ff7f0e"
COLOR_HYB = "#2ca02c"

# ============================================================
# Helper: plot one metric vs THREADS (medium image)
# X-axis: threads, Lines: Seq / Par / Hybrid
# ============================================================
def plot_metric_threads(metric_name, filename, y_label):
    threads_list = [3, 6, 8, 12, 15]
    x = np.array(threads_list, dtype=float)

    seq_vals = []
    par_vals = []
    hyb_vals = []

    for threads in threads_list:
        seq_t = T_seq_medium
        par_t = par_medium_by_threads[threads]
        hyb_t = hyb_medium_by_threads[threads]

        if metric_name == "time":
            seq_vals.append(seq_t)
            par_vals.append(par_t)
            hyb_vals.append(hyb_t)

        elif metric_name == "speedup":
            S_seq = 1.0
            S_par = T_seq_medium / par_t
            S_hyb = T_seq_medium / hyb_t
            seq_vals.append(S_seq)
            par_vals.append(S_par)
            hyb_vals.append(S_hyb)

        elif metric_name == "efficiency":
            p = threads
            S_seq = 1.0
            S_par = T_seq_medium / par_t
            S_hyb = T_seq_medium / hyb_t
            E_seq = S_seq / p
            E_par = S_par / p
            E_hyb = S_hyb / p
            seq_vals.append(E_seq)
            par_vals.append(E_par)
            hyb_vals.append(E_hyb)

        elif metric_name == "cost":
            p = threads
            C_seq = 1 * seq_t
            C_par = p * par_t
            C_hyb = p * hyb_t
            seq_vals.append(C_seq)
            par_vals.append(C_par)
            hyb_vals.append(C_hyb)

        elif metric_name == "throughput":
            Th_seq = PIX_MED / (seq_t * 1e6)
            Th_par = PIX_MED / (par_t * 1e6)
            Th_hyb = PIX_MED / (hyb_t * 1e6)
            seq_vals.append(Th_seq)
            par_vals.append(Th_par)
            hyb_vals.append(Th_hyb)

        else:
            raise ValueError("Unknown metric")

    plt.figure(figsize=(7, 5))
    plt.plot(x, seq_vals, marker="o", linewidth=2, color=COLOR_SEQ, label="Sequential")
    plt.plot(x, par_vals, marker="o", linewidth=2, color=COLOR_PAR, label="Parallel")
    plt.plot(x, hyb_vals, marker="o", linewidth=2, color=COLOR_HYB, label="Hybrid")

    plt.xlabel("Number of Threads", fontsize=12)
    plt.ylabel(y_label, fontsize=12)
    plt.title(f"{metric_name.capitalize()} vs Number of Threads\n(Medium image)", fontsize=13)
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.xticks(threads_list, [str(t) for t in threads_list])
    plt.legend(fontsize=10)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, filename))
    plt.show()


# ============================================================
# Helper: plot one metric vs PROCESSORS (fine image)
# X-axis: processors, Lines: Seq / Par / Hybrid
# ============================================================
def plot_metric_processors(metric_name, filename, y_label):
    procs_list = [2, 5, 9]
    x = np.array(procs_list, dtype=float)

    seq_vals = []
    par_vals = []
    hyb_vals = []

    for procs in procs_list:
        seq_t = T_seq_fine
        par_t = par_fine_by_procs[procs]
        hyb_t = hyb_fine_by_procs[procs]

        if metric_name == "time":
            seq_vals.append(seq_t)
            par_vals.append(par_t)
            hyb_vals.append(hyb_t)

        elif metric_name == "speedup":
            S_seq = 1.0
            S_par = T_seq_fine / par_t
            S_hyb = T_seq_fine / hyb_t
            seq_vals.append(S_seq)
            par_vals.append(S_par)
            hyb_vals.append(S_hyb)

        elif metric_name == "efficiency":
            p = procs
            S_seq = 1.0
            S_par = T_seq_fine / par_t
            S_hyb = T_seq_fine / hyb_t
            E_seq = S_seq / p
            E_par = S_par / p
            E_hyb = S_hyb / p
            seq_vals.append(E_seq)
            par_vals.append(E_par)
            hyb_vals.append(E_hyb)

        elif metric_name == "cost":
            p = procs
            C_seq = 1 * seq_t
            C_par = p * par_t
            C_hyb = p * hyb_t
            seq_vals.append(C_seq)
            par_vals.append(C_par)
            hyb_vals.append(C_hyb)

        elif metric_name == "throughput":
            Th_seq = PIX_FINE / (seq_t * 1e6)
            Th_par = PIX_FINE / (par_t * 1e6)
            Th_hyb = PIX_FINE / (hyb_t * 1e6)
            seq_vals.append(Th_seq)
            par_vals.append(Th_par)
            hyb_vals.append(Th_hyb)

        else:
            raise ValueError("Unknown metric")

    plt.figure(figsize=(7, 5))
    plt.plot(x, seq_vals, marker="o", linewidth=2, color=COLOR_SEQ, label="Sequential")
    plt.plot(x, par_vals, marker="o", linewidth=2, color=COLOR_PAR, label="Parallel")
    plt.plot(x, hyb_vals, marker="o", linewidth=2, color=COLOR_HYB, label="Hybrid")

    plt.xlabel("Number of Processors", fontsize=12)
    plt.ylabel(y_label, fontsize=12)
    plt.title(f"{metric_name.capitalize()} vs Number of Processors\n(Fine image)", fontsize=13)
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.xticks(procs_list, [str(p) for p in procs_list])
    plt.legend(fontsize=10)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, filename))
    plt.show()


# ============================================================
# Helper: plot one metric vs GRANULARITY (small/medium/fine, 8 threads)
# X-axis: granularity, Lines: Seq / Par / Hybrid
# ============================================================
def plot_metric_granularity(metric_name, filename, y_label):
    gran_labels = list(granularity_data_time.keys())  # ["Small","Medium","Fine"]
    x = np.arange(len(gran_labels))

    seq_vals = []
    par_vals = []
    hyb_vals = []

    for label in gran_labels:
        seq_t, par_t, hyb_t, pix = granularity_data_time[label]

        if metric_name == "time":
            seq_vals.append(seq_t)
            par_vals.append(par_t)
            hyb_vals.append(hyb_t)

        elif metric_name == "speedup":
            S_seq = 1.0
            S_par = seq_t / par_t
            S_hyb = seq_t / hyb_t
            seq_vals.append(S_seq)
            par_vals.append(S_par)
            hyb_vals.append(S_hyb)

        elif metric_name == "efficiency":
            p = 8  # fixed threads
            S_seq = 1.0
            S_par = seq_t / par_t
            S_hyb = seq_t / hyb_t
            E_seq = S_seq / p
            E_par = S_par / p
            E_hyb = S_hyb / p
            seq_vals.append(E_seq)
            par_vals.append(E_par)
            hyb_vals.append(E_hyb)

        elif metric_name == "cost":
            p = 8
            C_seq = 1 * seq_t
            C_par = p * par_t
            C_hyb = p * hyb_t
            seq_vals.append(C_seq)
            par_vals.append(C_par)
            hyb_vals.append(C_hyb)

        elif metric_name == "throughput":
            Th_seq = pix / (seq_t * 1e6)
            Th_par = pix / (par_t * 1e6)
            Th_hyb = pix / (hyb_t * 1e6)
            seq_vals.append(Th_seq)
            par_vals.append(Th_par)
            hyb_vals.append(Th_hyb)

        else:
            raise ValueError("Unknown metric")

    plt.figure(figsize=(7, 5))
    plt.plot(x, seq_vals, marker="o", linewidth=2, color=COLOR_SEQ, label="Sequential")
    plt.plot(x, par_vals, marker="o", linewidth=2, color=COLOR_PAR, label="Parallel")
    plt.plot(x, hyb_vals, marker="o", linewidth=2, color=COLOR_HYB, label="Hybrid")

    plt.xlabel("Granularity (Image Size)", fontsize=12)
    plt.ylabel(y_label, fontsize=12)
    plt.title(f"{metric_name.capitalize()} vs Granularity\n(8 threads)", fontsize=13)
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.xticks(x, gran_labels)
    plt.legend(fontsize=10)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, filename))
    plt.show()


# ============================================================
# MAIN: generate all 15 graphs
# ============================================================
if __name__ == "__main__":
    # 1) Execution Time
    plot_metric_threads("time", "1_time_threads.png", "Execution Time (s)")
    plot_metric_processors("time", "1_time_processors.png", "Execution Time (s)")
    plot_metric_granularity("time", "1_time_granularity.png", "Execution Time (s)")

    # 2) Speedup
    plot_metric_threads("speedup", "2_speedup_threads.png", "Speedup (T_seq / T)")
    plot_metric_processors("speedup", "2_speedup_processors.png", "Speedup (T_seq / T)")
    plot_metric_granularity("speedup", "2_speedup_granularity.png", "Speedup (T_seq / T)")

    # 3) Efficiency
    plot_metric_threads("efficiency", "3_efficiency_threads.png", "Efficiency (Speedup / p)")
    plot_metric_processors("efficiency", "3_efficiency_processors.png", "Efficiency (Speedup / p)")
    plot_metric_granularity("efficiency", "3_efficiency_granularity.png", "Efficiency (Speedup / p)")

    # 4) Cost
    plot_metric_threads("cost", "4_cost_threads.png", "Cost (p × Time)")
    plot_metric_processors("cost", "4_cost_processors.png", "Cost (p × Time)")
    plot_metric_granularity("cost", "4_cost_granularity.png", "Cost (p × Time)")

    # 5) Throughput
    plot_metric_threads("throughput", "5_throughput_threads.png", "Throughput (Mpixels / s)")
    plot_metric_processors("throughput", "5_throughput_processors.png", "Throughput (Mpixels / s)")
    plot_metric_granularity("throughput", "5_throughput_granularity.png", "Throughput (Mpixels / s)")

    print("All 15 metric plots generated in folder:", OUTPUT_DIR)
