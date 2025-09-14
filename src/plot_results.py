import sys
import csv
import matplotlib.pyplot as plt
from collections import defaultdict
import os


def load_csv(files):
    rows = []
    for f in files:
        with open(f) as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                # Separar por '|'
                parts = [p.strip() for p in line.split('|')]
                # Diccionario para los valores
                data = {}
                for p in parts:
                    if 'OpenMP' in p or 'std::thread' in p:
                        # Extraer api y variante
                        if 'OpenMP' in p:
                            data['api'] = 'OpenMP'
                        elif 'std::thread' in p:
                            data['api'] = 'std::thread'
                        # Variante
                        if 'variant' in p:
                            v = p.split('variant')[-1].strip()
                            data['variant'] = v.replace(':', '').strip()
                        else:
                            data['variant'] = ''
                    elif 'Threads' in p:
                        data['threads'] = int(p.split('=')[-1].strip())
                    elif 'Time' in p:
                        # Puede ser Time(us) o Time(us)=
                        val = p.split('=')[-1].replace('(us)', '').replace(')', '').strip()
                        data['us'] = float(val)
                    elif 'Sum' in p:
                        data['sum_hist'] = int(p.split('=')[-1].strip())
                # Solo agregar si tiene los campos necesarios
                if all(k in data for k in ['api', 'variant', 'threads', 'us', 'sum_hist']):
                    # Los campos faltantes se ponen en 0 o vacío
                    data['n'] = 0
                    data['bins'] = 0
                    data['seed'] = 0
                    data['checked'] = 0
                    rows.append(data)
    return rows


def plot_results(rows):
    # Crear carpeta plots si no existe
    os.makedirs("plots", exist_ok=True)
    groups = defaultdict(list)
    for r in rows:
        groups[(r["api"], r["variant"])].append(r)

    # Sort each group by number of threads
    for key in groups:
        groups[key].sort(key=lambda r: r["threads"])

    # baseline for speedup
    baselines = {k: min(v, key=lambda r: r["threads"]) for k, v in groups.items()}

    # Plot execution time vs threads
    plt.figure()
    for key, lst in groups.items():
        x = [r["threads"] for r in lst]
        y = [r["us"] for r in lst]
        plt.plot(x, y, marker='o', label=f"{key[0]} - {key[1]}")
    plt.xlabel("Threads")
    plt.ylabel("Time (us)")
    plt.title("Execution Time vs Threads")
    plt.legend()
    plt.grid(True)
    plt.savefig("plots/time_vs_threads.png", dpi=150)

    # Speedup
    plt.figure()
    for key, lst in groups.items():
        base = baselines[key]
        base_time = base["us"]
        x = [r["threads"] for r in lst]
        y = [base_time / r["us"] for r in lst]
        plt.plot(x, y, marker="o", label=f"{key[0]} - {key[1]}")
    plt.xlabel("Threads")
    plt.ylabel("Speedup")
    plt.title("Speedup vs Threads")
    plt.legend()
    plt.grid(True)
    plt.savefig("plots/speedup_vs_threads.png", dpi=150)

    # Efficiency
    plt.figure()
    for key, lst in groups.items():
        base = baselines[key]
        base_time = base["us"]
        x = [r["threads"] for r in lst]
        y = [(base_time / r["us"]) / r["threads"] for r in lst]
        plt.plot(x, y, marker="o", label=f"{key[0]} - {key[1]}")
    plt.xlabel("Threads")
    plt.ylabel("Efficiency")
    plt.title("Efficiency vs Threads")
    plt.legend()
    plt.grid(True)
    plt.savefig("plots/efficiency_vs_threads.png", dpi=150)

    print("✅ Plots saved in 'plots/' folder: time_vs_threads.png, speedup_vs_threads.png, efficiency_vs_threads.png")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python3 plot_results.py resultados.csv [mas.csv ...]")
        sys.exit(1)

    rows = load_csv(sys.argv[1:])
    plot_results(rows)