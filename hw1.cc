// #define DEBUG
#include <mpi.h>

#include <algorithm>
#include <boost/sort/spreadsort/spreadsort.hpp>
#include <cmath>
#include <compare>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>

#define MAXN 536870911
#define MIN_PROC_N 1
#ifdef DEBUG
#define MPI_EXECUTE(func)                                          \
    {                                                              \
        int rc = func;                                             \
        if (rc != MPI_SUCCESS) {                                   \
            printf("Error on MPI function at line %d.", __LINE__); \
            MPI_Abort(MPI_COMM_WORLD, rc);                         \
        }                                                          \
    }
#define DEBUG_PRINT(fmt, args...)     \
    do {                              \
        fprintf(stderr, fmt, ##args); \
    } while (false);
#else
#define MPI_EXECUTE(func) \
    {                     \
        func;             \
    }
#define DEBUG_PRINT(fmt, args...) \
    do {                          \
    } while (false);
#endif

void MPI_merge(int ln, float *&larr, int rn, float *&rarr, float *&tmparr);
void MPI_merge_low(int ln, float *&larr, int rn, float *rarr, float *&tmparr);
void MPI_merge_high(int ln, float *larr, int rn, float *&rarr, float *&tmparr);
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int array_size = std::stoi(argv[1]);
    char *input_filename = argv[2];
    char *output_filename = argv[3];

    MPI_File input_file, output_file;
    int target_world_size;
    int step, remain, count, start;
    int odd_rank = -1, even_rank = -1;
    int odd_count = 0, even_count = 0;
    float recv_val;
    float *local_data = NULL, *recv_data = NULL, *temp_data = NULL;
    bool phase_sorted;
#ifdef DEBUG
    double start_time, duration, sum_duration, max_duration;
    double sendrecv_start_time, sendrecv_duration, sum_sendrecv_duration, max_sendrecv_duration;
    double MPI_merge_start_time, MPI_merge_duration, sum_MPI_merge_duration, max_MPI_merge_duration;
#endif

    /* Calculate index */
    target_world_size = std::max(std::min(world_size, array_size / MIN_PROC_N), 1);
    step = array_size / target_world_size;
    remain = array_size % target_world_size;
    if (world_rank < remain) {
        count = step + 1;
        start = world_rank * (step + 1);
    } else if (world_rank < target_world_size) {
        count = step;
        start = remain * (step + 1) + (world_rank - remain) * step;
    } else {
        count = 0;
        start = array_size;
    }

    if (world_rank & 1) {
        odd_rank = world_rank + 1;
        even_rank = world_rank - 1;
    } else {
        odd_rank = world_rank - 1;
        even_rank = world_rank + 1;
    }
    if (odd_rank < 0 || odd_rank >= target_world_size || world_rank >= target_world_size) {
        odd_rank = MPI_PROC_NULL;
    }
    if (even_rank < 0 || even_rank >= target_world_size || world_rank >= target_world_size) {
        even_rank = MPI_PROC_NULL;
    }

    if (odd_rank == MPI_PROC_NULL) {
        odd_count = 0;
    } else {
        if (odd_rank < remain)
            odd_count = step + 1;
        else
            odd_count = step;
    }

    if (even_rank == MPI_PROC_NULL) {
        even_count = 0;
    } else {
        if (even_rank < remain)
            even_count = step + 1;
        else
            even_count = step;
    }

    DEBUG_PRINT("rank: %d %d %d, count: %d %d %d\n", world_rank, odd_rank, even_rank, count, odd_count, even_count);
    /* Read input */
#ifdef DEBUG
    start_time = MPI_Wtime();
#endif
    local_data = new float[step + 1];
    MPI_File_open(MPI_COMM_WORLD, input_filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);
    MPI_File_read_at(input_file, sizeof(float) * start, local_data, count, MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&input_file);
#ifdef DEBUG
    duration = MPI_Wtime() - start_time;
    MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        DEBUG_PRINT("Read IO: %lf, %lf\n", sum_duration / world_size, max_duration);
    }
#endif

    /* Local sort */
#ifdef DEBUG
    start_time = MPI_Wtime();
#endif
    boost::sort::spreadsort::spreadsort(local_data, local_data + count);
#ifdef DEBUG
    duration = MPI_Wtime() - start_time;
    MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        DEBUG_PRINT("Local sort: %lf, %lf\n", sum_duration / world_size, max_duration);
    }
#endif

    /* Sorting */
#ifdef DEBUG
    start_time = MPI_Wtime();
    sendrecv_duration = 0.0;
#endif
    temp_data = new float[step + 1];
    recv_data = new float[step + 1];
#pragma GCC unroll 50
    for (int p = 0; p < target_world_size + 1; p++) {
        if (p & 1) {
            if (odd_rank != MPI_PROC_NULL) { /* Odd phase */
                phase_sorted = true;
                if (world_rank & 1) {
                    MPI_Sendrecv(local_data + count - 1, 1, MPI_FLOAT, odd_rank, 0, &recv_val, 1, MPI_FLOAT, odd_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (local_data[count - 1] > recv_val)
                        phase_sorted = false;
                } else {
                    MPI_Sendrecv(local_data, 1, MPI_FLOAT, odd_rank, 0, &recv_val, 1, MPI_FLOAT, odd_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (local_data[0] < recv_val)
                        phase_sorted = false;
                }
                if (!phase_sorted) {
#ifdef DEBUG
                    sendrecv_start_time = MPI_Wtime();
#endif
                    MPI_Sendrecv(local_data, count, MPI_FLOAT, odd_rank, 0, recv_data, odd_count, MPI_FLOAT, odd_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#ifdef DEBUG
                    sendrecv_duration += MPI_Wtime() - sendrecv_start_time;
                    MPI_merge_start_time = MPI_Wtime();
#endif
                    if (world_rank & 1) {
                        MPI_merge_low(count, local_data, odd_count, recv_data, temp_data);
                    } else {
                        MPI_merge_high(odd_count, recv_data, count, local_data, temp_data);
                    }
#ifdef DEBUG
                    MPI_merge_duration += MPI_Wtime() - MPI_merge_start_time;
#endif
                }
            }
        } else {
            if (even_rank != MPI_PROC_NULL) { /* Even phase */
                phase_sorted = true;
                if (world_rank & 1) {
                    MPI_Sendrecv(local_data, 1, MPI_FLOAT, even_rank, 0, &recv_val, 1, MPI_FLOAT, even_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (local_data[0] < recv_val)
                        phase_sorted = false;
                } else {
                    MPI_Sendrecv(local_data + count - 1, 1, MPI_FLOAT, even_rank, 0, &recv_val, 1, MPI_FLOAT, even_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (local_data[count - 1] > recv_val)
                        phase_sorted = false;
                }
                if (!phase_sorted) {
#ifdef DEBUG
                    sendrecv_start_time = MPI_Wtime();
#endif
                    MPI_Sendrecv(local_data, count, MPI_FLOAT, even_rank, 0, recv_data, even_count, MPI_FLOAT, even_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#ifdef DEBUG
                    sendrecv_duration += MPI_Wtime() - sendrecv_start_time;
                    MPI_merge_start_time = MPI_Wtime();
#endif
                    if (world_rank & 1) {
                        MPI_merge_high(even_count, recv_data, count, local_data, temp_data);
                    } else {
                        MPI_merge_low(count, local_data, even_count, recv_data, temp_data);
                    }
#ifdef DEBUG
                    MPI_merge_duration += MPI_Wtime() - MPI_merge_start_time;
#endif
                }
            }
        }
    }
#ifdef DEBUG
    duration = MPI_Wtime() - start_time;
    MPI_Reduce(&sendrecv_duration, &sum_sendrecv_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&sendrecv_duration, &max_sendrecv_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        DEBUG_PRINT("Sendrecv: %lf, %lf\n", sum_sendrecv_duration / world_size, max_sendrecv_duration);
    }
    MPI_Reduce(&MPI_merge_duration, &sum_MPI_merge_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&MPI_merge_duration, &max_MPI_merge_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        DEBUG_PRINT("merging: %lf, %lf\n", sum_MPI_merge_duration / world_size, max_MPI_merge_duration);
    }
    MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        DEBUG_PRINT("Global sort: %lf, %lf\n", sum_duration / world_size, max_duration);
    }
#endif

    /* Write output */
#ifdef DEBUG
    start_time = MPI_Wtime();
#endif
    MPI_File_open(MPI_COMM_WORLD, output_filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &output_file);
    MPI_File_write_at(output_file, sizeof(float) * start, local_data, count, MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&output_file);
#ifdef DEBUG
    duration = MPI_Wtime() - start_time;
    MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        DEBUG_PRINT("Output IO: %lf, %lf\n", sum_duration / world_size, max_duration);
    }
#endif

    /* Finalize program */
    delete[] temp_data;
    delete[] recv_data;
    delete[] local_data;
    MPI_Finalize();
    return 0;
}

void MPI_merge(int ln, float *larr, int rn, float *rarr, float *tmparr) {
    int li, ri, ti;
    li = ri = ti = 0;
    while (li < ln && ri < rn) {
        if (larr[li] <= rarr[ri]) {
            tmparr[ti++] = larr[li++];
        } else {
            tmparr[ti++] = rarr[ri++];
        }
    }
    while (li < ln) {
        tmparr[ti++] = larr[li++];
    }
    while (ri < rn) {
        tmparr[ti++] = rarr[ri++];
    }

    li = ri = ti = 0;
    while (li < ln) {
        larr[li++] = tmparr[ti++];
    }
    while (ri < rn) {
        rarr[ri++] = tmparr[ti++];
    }
    return;
}

void MPI_merge_low(int ln, float *&larr, int rn, float *rarr, float *&tmparr) {
    if (larr[ln - 1] <= rarr[0])
        return;
    int li, ri, ti;
    li = ri = ti = 0;
    while (li < ln && ri < rn && ti < ln) {
        if (larr[li] <= rarr[ri]) {
            tmparr[ti++] = larr[li++];
        } else {
            tmparr[ti++] = rarr[ri++];
        }
    }
    while (li < ln && ti < ln) {
        tmparr[ti++] = larr[li++];
    }
    while (ri < rn && ti < ln) {
        tmparr[ti++] = rarr[ri++];
    }
    std::swap(larr, tmparr);
    return;
}

void MPI_merge_high(int ln, float *larr, int rn, float *&rarr, float *&tmparr) {
    if (larr[ln - 1] <= rarr[0])
        return;
    int li, ri, ti;
    li = ln - 1;
    ti = ri = rn - 1;
    while (li >= 0 && ri >= 0 && ti >= 0) {
        if (larr[li] <= rarr[ri]) {
            tmparr[ti--] = rarr[ri--];
        } else {
            tmparr[ti--] = larr[li--];
        }
    }
    while (li >= 0 && ti >= 0) {
        tmparr[ti--] = larr[li--];
    }
    while (ri >= 0 && ti >= 0) {
        tmparr[ti--] = rarr[ri--];
    }
    std::swap(rarr, tmparr);
    return;
}