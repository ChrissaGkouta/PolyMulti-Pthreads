import subprocess
import re

# Παράμετροι πειραμάτων
degrees = [1000, 5000, 10000] # Προσοχή: Το 10^5 είναι πολύ αργό για O(N^2)
threads_list = [1, 2, 4, 8]
runs_per_experiment = 4

def run_cmd(n, t):
    result = subprocess.run(["./poly_mult", str(n), str(t)], capture_output=True, text=True)
    return result.stdout

print(f"{'Degree':<10} | {'Threads':<8} | {'Avg Serial (s)':<15} | {'Avg Parallel (s)':<15} | {'Speedup':<10}")
print("-" * 70)

for n in degrees:
    for t in threads_list:
        serial_times = []
        parallel_times = []
        
        for _ in range(runs_per_experiment):
            output = run_cmd(n, t)
            
            # Εξαγωγή χρόνων με regex
            s_time = float(re.search(r"Serial Time: ([0-9.]+)", output).group(1))
            p_time = float(re.search(r"Parallel Time: ([0-9.]+)",  output).group(1))
            
            serial_times.append(s_time)
            parallel_times.append(p_time) 
        
        avg_serial = sum(serial_times) / runs_per_experiment
        avg_parallel = sum(parallel_times) / runs_per_experiment
        speedup = avg_serial / avg_parallel if avg_parallel > 0 else 0
        
        print(f"{n:<10} | {t:<8} | {avg_serial:<15.4f} | {avg_parallel:<15.4f} | {speedup:<10.2f}")