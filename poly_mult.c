#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
//#include <time.h>
#include <string.h>
#include <linux/time.h>

// Δομή για τα ορίσματα που περνάμε σε κάθε νήμα
typedef struct {
    int thread_id;
    int num_threads;
    int n;          // Βαθμός πολυωνύμου
    int *A;         // Πολυώνυμο A
    int *B;         // Πολυώνυμο B
    long long *C;   // Πολυώνυμο αποτελέσματος (long long για αποφυγή overflow)
} thread_args_t;

// Βοηθητική συνάρτηση για μέτρηση χρόνου
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Σειριακός Αλγόριθμος (Ο(n^2))
void serial_mult(int n, int *A, int *B, long long *C) {
    // Μηδενισμός του πίνακα αποτελεσμάτων
    for (int i = 0; i <= 2 * n; i++) C[i] = 0;

    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= n; j++) {
            C[i + j] += (long long)A[i] * B[j];
        }
    }
}

// Συνάρτηση που εκτελεί κάθε νήμα
void *worker(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int n = args->n;
    int total_coeffs = 2 * n + 1; // Το αποτέλεσμα έχει βαθμό 2n, άρα 2n+1 όρους

    // Υπολογισμός του εύρους δεικτών (k) που αναλογούν σε αυτό το νήμα
    // Διαμερίζουμε το τελικό πολυώνυμο C[k]
    int chunk = total_coeffs / args->num_threads;
    int remainder = total_coeffs % args->num_threads;
    
    int start_k = args->thread_id * chunk + (args->thread_id < remainder ? args->thread_id : remainder);
    int end_k = start_k + chunk + (args->thread_id < remainder ? 1 : 0);

    // Υπολογισμός συντελεστών C[k] για το εύρος που ανατέθηκε
    for (int k = start_k; k < end_k; k++) {
        long long sum = 0;
        
        // Για να βρούμε το C[k], πρέπει να αθροίσουμε τα A[i]*B[j] όπου i+j = k
        // Περιορισμοί: 0 <= i <= n ΚΑΙ 0 <= j <= n
        // j = k - i => 0 <= k - i <= n => i <= k ΚΑΙ i >= k - n
        
        int start_i = (k - n > 0) ? (k - n) : 0;
        int end_i = (k < n) ? k : n;

        for (int i = start_i; i <= end_i; i++) {
            sum += (long long)args->A[i] * args->B[k - i];
        }
        args->C[k] = sum;
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <degree n> <num_threads>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    struct timespec start, end;
    double t_init, t_serial, t_parallel;

    // --- 1. Initialization ---
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int *A = (int *)malloc((n + 1) * sizeof(int));
    int *B = (int *)malloc((n + 1) * sizeof(int));
    long long *C_serial = (long long *)malloc((2 * n + 1) * sizeof(long long));
    long long *C_parallel = (long long *)malloc((2 * n + 1) * sizeof(long long));

    if (!A || !B || !C_serial || !C_parallel) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 1;
    }

    // Initialize with random non-zero integers
    srand(time(NULL));
    for (int i = 0; i <= n; i++) {
        do { A[i] = rand() % 100 - 50; } while (A[i] == 0);
        do { B[i] = rand() % 100 - 50; } while (B[i] == 0);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    t_init = get_time_diff(start, end);

    // --- 2. Serial Execution ---
    clock_gettime(CLOCK_MONOTONIC, &start);
    serial_mult(n, A, B, C_serial);
    clock_gettime(CLOCK_MONOTONIC, &end);
    t_serial = get_time_diff(start, end);

    // --- 3. Parallel Execution ---
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    thread_args_t *args = malloc(num_threads * sizeof(thread_args_t));

    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].num_threads = num_threads;
        args[i].n = n;
        args[i].A = A;
        args[i].B = B;
        args[i].C = C_parallel;
        pthread_create(&threads[i], NULL, worker, (void *)&args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    t_parallel = get_time_diff(start, end);

    // --- 4. Verification ---
    int correct = 1;
    for (int i = 0; i <= 2 * n; i++) {
        if (C_serial[i] != C_parallel[i]) {
            correct = 0;
            printf("Mismatch at index %d: Serial=%lld, Parallel=%lld\n", i, C_serial[i], C_parallel[i]);
            break;
        }
    }

    // --- Output ---
    printf("Degree: %d, Threads: %d\n", n, num_threads);
    printf("Init Time: %.6f sec\n", t_init);
    printf("Serial Time: %.6f sec\n", t_serial);
    printf("Parallel Time: %.6f sec\n", t_parallel);
    printf("Verification: %s\n", correct ? "SUCCESS" : "FAILURE");

    // Cleanup
    free(A); free(B); free(C_serial); free(C_parallel);
    free(threads); free(args);

    return 0;
}