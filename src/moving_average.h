#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the window size for the moving average
#define WINDOW_SIZE 5
#define MAX_DIMENSIONS 10000  // Maximum number of dimensions supported
#ifndef MA_TYPE
#define MA_TYPE float
#endif

// Structure to hold n-dimensional moving average state
typedef struct {
	int dimensions;                           // Number of dimensions
	MA_TYPE **data_buffer;                     // 2D array: [WINDOW_SIZE][dimensions]
	MA_TYPE *current_sum;                      // Running sum for each dimension
	int buffer_index;                        // Current position in the circular buffer
	int data_count;                          // Number of data points currently in the buffer
} MovingAverageND;

// Initialize the n-dimensional moving average structure
MovingAverageND* init_moving_average_nd(int dimensions) {
	if (dimensions <= 0 || dimensions > MAX_DIMENSIONS) {
		printf("Error: Invalid number of dimensions (1-%d allowed)\n", MAX_DIMENSIONS);
		return NULL;
	}

	MovingAverageND *ma = (MovingAverageND*)malloc(sizeof(MovingAverageND));
	if (!ma) return NULL;

	ma->dimensions = dimensions;
	ma->buffer_index = 0;
	ma->data_count = 0;

	// Allocate memory for the data buffer
	ma->data_buffer = (MA_TYPE**)malloc(WINDOW_SIZE * sizeof(MA_TYPE*));
	if (!ma->data_buffer) {
		free(ma);
		return NULL;
	}

	for (int i = 0; i < WINDOW_SIZE; i++) {
		ma->data_buffer[i] = (MA_TYPE*)calloc(dimensions, sizeof(MA_TYPE));
		if (!ma->data_buffer[i]) {
			// Clean up on allocation failure
			for (int j = 0; j < i; j++) {
				free(ma->data_buffer[j]);
			}
			free(ma->data_buffer);
			free(ma);
			return NULL;
		}
	}

	// Allocate and initialize the sum array
	ma->current_sum = (MA_TYPE*)calloc(dimensions, sizeof(MA_TYPE));
	if (!ma->current_sum) {
		for (int i = 0; i < WINDOW_SIZE; i++) {
			free(ma->data_buffer[i]);
		}
		free(ma->data_buffer);
		free(ma);
		return NULL;
	}

	return ma;
}

// Calculate moving average for n-dimensional data
void calculate_moving_average_nd(MovingAverageND *ma, const MA_TYPE *new_values, MA_TYPE *result) {
	if (!ma || !new_values || !result) return;

	// If the buffer is full, remove the oldest values from the sums
	if (ma->data_count == WINDOW_SIZE) {
		for (int dim = 0; dim < ma->dimensions; dim++) {
			ma->current_sum[dim] -= ma->data_buffer[ma->buffer_index][dim];
		}
	} else {
		ma->data_count++; // Increment count until buffer is full
	}

	// Add the new values to the buffer and the sums
	for (int dim = 0; dim < ma->dimensions; dim++) {
		ma->data_buffer[ma->buffer_index][dim] = new_values[dim];
		ma->current_sum[dim] += new_values[dim];
	}

	// Move to the next position in the circular buffer
	ma->buffer_index = (ma->buffer_index + 1) % WINDOW_SIZE;

	// Calculate and store the moving averages for each dimension
	for (int dim = 0; dim < ma->dimensions; dim++) {
		result[dim] = ma->current_sum[dim] / ma->data_count;
	}
}

// Clean up the moving average structure
void free_moving_average_nd(MovingAverageND *ma) {
	if (!ma) return;

	if (ma->data_buffer) {
		for (int i = 0; i < WINDOW_SIZE; i++) {
			free(ma->data_buffer[i]);
		}
		free(ma->data_buffer);
	}

	free(ma->current_sum);
	free(ma);
}

// Helper function to print n-dimensional data
void print_nd_data(const MA_TYPE *data, int dimensions, const char *label) {
	printf("%s: [", label);
	for (int i = 0; i < dimensions; i++) {
		printf("%.2f", data[i]);
		if (i < dimensions - 1) printf(", ");
	}
	printf("]\n");
}

// Original 1D function for backward compatibility
MA_TYPE calculate_moving_average_1d(MA_TYPE new_value) {
	static MA_TYPE data_buffer[WINDOW_SIZE] = {0};
	static MA_TYPE current_sum = 0;
	static int buffer_index = 0;
	static int data_count = 0;

	if (data_count == WINDOW_SIZE) {
		current_sum -= data_buffer[buffer_index];
	} else {
		data_count++;
	}

	data_buffer[buffer_index] = new_value;
	current_sum += new_value;

	buffer_index = (buffer_index + 1) % WINDOW_SIZE;

	return current_sum / data_count;
}
