// OS 24 EX1

#include <fstream>
#include "memory_latency.h"
#include "measure.h"

#define GALOIS_POLYNOMIAL ((1ULL << 63) | (1ULL << 62) | (1ULL << 60) | (1ULL << 59))

/**
 * Converts the struct timespec to time in nano-seconds.
 * @param t - the struct timespec to convert.
 * @return - the value of time in nano-seconds.
 */
uint64_t nanosectime(struct timespec t)
{
    return t.tv_sec*1000000000 + t.tv_nsec;
}

/**
* Measures the average latency of accessing a given array in a sequential order.
* @param repeat - the number of times to repeat the measurement for and average on.
* @param arr - an allocated (not empty) array to preform measurement on.
* @param arr_size - the length of the array arr.
* @param zero - a variable containing zero in a way that the compiler doesn't "know" it in compilation time.
* @return struct measurement containing the measurement with the following fields:
*      double baseline - the average time (ns) taken to preform the measured operation without memory access.
*      double access_time - the average time (ns) taken to preform the measured operation with memory access.
*      uint64_t rnd - the variable used to randomly access the array, returned to prevent compiler optimizations.
*/
struct measurement measure_sequential_latency(uint64_t repeat, array_element_t* arr, uint64_t arr_size, uint64_t zero) {
    repeat = arr_size > repeat ? arr_size : repeat; // Ensure repeat >= arr_size

    // Baseline measurement:
    struct timespec t0;
    timespec_get(&t0, TIME_UTC);
    register uint64_t rnd = 12345;
    for (register uint64_t i = 0; i < repeat; i++) {
        register uint64_t index = rnd % arr_size; // Sequential access pattern
        rnd ^= index & zero;
        rnd += 1;
    }
    struct timespec t1;
    timespec_get(&t1, TIME_UTC);

    // Memory access measurement:
    struct timespec t2;
    timespec_get(&t2, TIME_UTC);
    rnd = (rnd & zero) ^ 12345;
    for (register uint64_t i = 0; i < repeat; i++) {
        register uint64_t index = rnd % arr_size; // Sequential access pattern
        rnd ^= arr[index] & zero;
        rnd += 1;
    }
        struct timespec t3;
        timespec_get(&t3, TIME_UTC);

        // Calculate baseline and memory access times:
        double baseline_per_cycle = (double) (nanosectime(t1) - nanosectime(t0)) / repeat;
        double memory_per_cycle = (double) (nanosectime(t3) - nanosectime(t2)) / repeat;
        struct measurement result;

        result.baseline = baseline_per_cycle;
        result.access_time = memory_per_cycle;
        result.rnd = rnd;
        return result;
}

    int main(int argc, char *argv[]) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s max_size factor repeat\n", argv[0]);
            return -1;
        }

        // Parse command-line arguments
        uint64_t max_size = strtoull(argv[1], NULL, 10);
        double factor = strtod(argv[2], NULL);
        uint64_t repeat = strtoull(argv[3], NULL, 10);

        if (max_size < 100 || factor <= 1 || repeat <= 0) {
            fprintf(stderr, "Invalid input arguments\n");
            return -1;
        }

        struct timespec t_dummy;
        timespec_get(&t_dummy, TIME_UTC);
        const uint64_t zero = nanosectime(t_dummy) > 1000000000ULL ? 0 : nanosectime(t_dummy);

        uint64_t array_size_in_memory = 100;
        while (array_size_in_memory <= max_size) {
            array_element_t *array = (array_element_t *) calloc(array_size_in_memory, sizeof(char));
            if (!array) {
                fprintf(stderr, "Failed to allocate memory of size %lu bytes\n", array_size_in_memory);
                return -1;
            }
            struct measurement random_measurement = measure_latency(repeat, array,
                                                                    array_size_in_memory / sizeof(array_element_t),
                                                                    zero);
            double random_offset = random_measurement.access_time - random_measurement.baseline;

            struct measurement sequential_measurement = measure_sequential_latency(repeat, array, array_size_in_memory /
                                                                                                  sizeof(array_element_t)
                                                                                                  ,zero);
            double sequential_offset = sequential_measurement.access_time - sequential_measurement.baseline;

            printf("%lu,%.2f,%.2f\n", array_size_in_memory, random_offset, sequential_offset);
            free(array);
            array = NULL;

            array_size_in_memory = (uint64_t) (array_size_in_memory * factor);
        }

        return 0;
    }
