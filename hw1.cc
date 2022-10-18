#define DEBUG
#include <mpi.h>

#include <algorithm>
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

int compare(const void *a, const void *b);
bool merge(int ln, float *larr, int rn, float *rarr, float *tmparr);
bool merge_low(int ln, float *larr, int rn, float *rarr, float *tmparr);
bool merge_high(int ln, float *larr, int rn, float *rarr, float *tmparr);
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
    float *local_data = NULL, *recv_data = NULL, *temp_data = NULL;
    bool all_sorted, part_sorted;
    double start_time, duration, sum_duration, max_duration;

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

    if (world_rank % 2 == 0) {
        odd_rank = world_rank - 1;
        even_rank = world_rank + 1;
    } else {
        odd_rank = world_rank + 1;
        even_rank = world_rank - 1;
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
    // start_time = MPI_Wtime();
    local_data = new float[count];
    MPI_File_open(MPI_COMM_WORLD, input_filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);
    MPI_File_read_at(input_file, sizeof(float) * start, local_data, count, MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&input_file);
    // duration = MPI_Wtime() - start_time;
    // MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    // MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    // if (world_rank == 0) {
    //     DEBUG_PRINT("Read IO: %lf, %lf\n", sum_duration / world_size, max_duration);
    // }

    /* Local sort */
    // start_time = MPI_Wtime();
    std::sort(local_data, local_data + count);
    // std::qsort(local_data, count, sizeof(float), compare);
    // duration = MPI_Wtime() - start_time;
    // MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    // MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    // if (world_rank == 0) {
    //     DEBUG_PRINT("Local sort: %lf, %lf\n", sum_duration / world_size, max_duration);
    // }

    /* Sorting */
    // start_time = MPI_Wtime();
    temp_data = new float[step + 1];
    recv_data = new float[step + 1];
    all_sorted = false;
    while (!all_sorted) {
        part_sorted = true;
        if (odd_rank != MPI_PROC_NULL) { /* Odd phase */
            MPI_Sendrecv(local_data, count, MPI_FLOAT, odd_rank, 0, recv_data, odd_count, MPI_FLOAT, odd_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (world_rank < odd_rank) {
                if (!merge_low(count, local_data, odd_count, recv_data, temp_data))
                    part_sorted = false;
            } else {
                if (!merge_high(odd_count, recv_data, count, local_data, temp_data))
                    part_sorted = false;
            }
        }
        if (even_rank != MPI_PROC_NULL) { /* Even phase */
            MPI_Sendrecv(local_data, count, MPI_FLOAT, even_rank, 0, recv_data, even_count, MPI_FLOAT, even_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (world_rank < even_rank) {
                if (!merge_low(count, local_data, even_count, recv_data, temp_data))
                    part_sorted = false;
            } else {
                if (!merge_high(even_count, recv_data, count, local_data, temp_data))
                    part_sorted = false;
            }
        }
        MPI_EXECUTE(MPI_Allreduce(&part_sorted, &all_sorted, 1, MPI_CXX_BOOL, MPI_LAND, MPI_COMM_WORLD));
    }
    // duration = MPI_Wtime() - start_time;
    // MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    // MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    // if (world_rank == 0) {
    //     DEBUG_PRINT("Global sort: %lf, %lf\n", sum_duration / world_size, max_duration);
    // }

    // #ifdef DEBUG
    //     for (int i = 0; i < count; i++) {
    //         DEBUG_PRINT("rank %d: %f\n", world_rank, local_data[i]);
    //     }
    // #endif

    /* Write output */
    // start_time = MPI_Wtime();
    MPI_File_open(MPI_COMM_WORLD, output_filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &output_file);
    MPI_File_write_at(output_file, sizeof(float) * start, local_data, count, MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&output_file);
    // duration = MPI_Wtime() - start_time;
    // MPI_Reduce(&duration, &sum_duration, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    // MPI_Reduce(&duration, &max_duration, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    // if (world_rank == 0) {
    //     DEBUG_PRINT("Output IO: %lf, %lf\n", sum_duration / world_size, max_duration);
    // }

    /* Finalize program */
    delete[] temp_data;
    delete[] recv_data;
    delete[] local_data;
    MPI_Finalize();
    return 0;
}

int compare(const void *a, const void *b) {
    float val1 = *(float *)a;
    float val2 = *(float *)b;
    if (val1 > val2)
        return 1;
    else if (val1 == val2)
        return 0;
    else
        return -1;
}

bool merge(int ln, float *larr, int rn, float *rarr, float *tmparr) {
    int li, ri, ti;
    bool retVal = true;
    li = ri = ti = 0;
    while (li < ln && ri < rn) {
        if (larr[li] <= rarr[ri]) {
            tmparr[ti++] = larr[li++];
        } else {
            tmparr[ti++] = rarr[ri++];
            retVal = false;
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
    return retVal;
}

bool merge_low(int ln, float *larr, int rn, float *rarr, float *tmparr) {
    int li, ri, ti;
    bool retVal = true;
    li = ri = ti = 0;
    while (li < ln && ri < rn && ti < ln) {
        if (larr[li] <= rarr[ri]) {
            tmparr[ti++] = larr[li++];
        } else {
            tmparr[ti++] = rarr[ri++];
            retVal = false;
        }
    }
    while (li < ln && ti < ln) {
        tmparr[ti++] = larr[li++];
    }
    while (ri < rn && ti < ln) {
        tmparr[ti++] = rarr[ri++];
    }

    for (li = 0; li < ln; li++) {
        larr[li] = tmparr[li];
    }
    return retVal;
}

bool merge_high(int ln, float *larr, int rn, float *rarr, float *tmparr) {
    int li, ri, ti;
    bool retVal = true;
    li = ln - 1;
    ti = ri = rn - 1;
    while (li >= 0 && ri >= 0 && ti >= 0) {
        if (larr[li] <= rarr[ri]) {
            tmparr[ti--] = rarr[ri--];
        } else {
            tmparr[ti--] = larr[li--];
            retVal = false;
        }
    }
    while (li >= 0 && ti >= 0) {
        tmparr[ti--] = larr[li--];
    }
    while (ri >= 0 && ti >= 0) {
        tmparr[ti--] = rarr[ri--];
    }

    for (ri = 0; ri < rn; ri++) {
        rarr[ri] = tmparr[ri];
    }
    return retVal;
}