#include <mpi.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <time.h>

double communication = 0;

double fRand(double fMin, double fMax)
{
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}

std::vector<double> make_matrix(int size) {
	std::vector<double> matrix(size * size);
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			matrix[i * size + j] = fRand(0, 10000);
		}
	}
	return matrix;
}

std::vector<double> make_vector(int size) {
	std::vector<double> vec(size);
	for (int i = 0; i < size; i++) {
		vec[i] = fRand(0, 10000);
	}
	return vec;
}

void send_str_and_vec(std::vector<double> matrix, std::vector<double> vec, int size, int matrix_size) {
	
	int rows_per_process = matrix_size;
	int remaining_rows = 0;

	if (size > 1)
	{
		rows_per_process = matrix_size / size;
		remaining_rows = matrix_size % size;
	}
		
	int current_rows = rows_per_process;

	int start_row_index = current_rows * matrix_size;

	for (int dest_rank = 1; dest_rank < size; dest_rank++) {
		
		if (dest_rank == size - 1)
			current_rows += remaining_rows; 
		
		double st = MPI_Wtime();

		MPI_Send(&matrix[start_row_index], current_rows * matrix_size, MPI_DOUBLE, dest_rank, 123, MPI_COMM_WORLD);
		MPI_Send(&vec[0], vec.size(), MPI_DOUBLE, dest_rank, 456, MPI_COMM_WORLD);

		double end = MPI_Wtime();

		communication += end - st;

		start_row_index += current_rows * matrix_size;
	}

}

std::vector<double> recv_string() {
	MPI_Status status;
	MPI_Probe(MPI_ANY_SOURCE, 123, MPI_COMM_WORLD, &status);
	int src_rank = status.MPI_SOURCE;
	int tag = status.MPI_TAG;
	int col_size;
	MPI_Get_count(&status, MPI_DOUBLE, &col_size);

	std::vector<double> column(col_size);
	MPI_Recv(&column[0], col_size, MPI_DOUBLE, src_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	return column;
}

std::vector<double> recv_vec() {
	MPI_Status status;
	MPI_Probe(MPI_ANY_SOURCE, 456, MPI_COMM_WORLD, &status);
	int src_rank = status.MPI_SOURCE;
	int tag = status.MPI_TAG;
	int vec_size;
	MPI_Get_count(&status, MPI_DOUBLE, &vec_size);

	std::vector<double> vec(vec_size);
	MPI_Recv(&vec[0], vec_size, MPI_DOUBLE, src_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	return vec;
}

std::vector<double> recv_res(int rank) {
	MPI_Status status;
	MPI_Probe(rank, 789, MPI_COMM_WORLD, &status);
	int src_rank = status.MPI_SOURCE;
	int tag = status.MPI_TAG;
	int res_size;
	MPI_Get_count(&status, MPI_DOUBLE, &res_size);

	std::vector<double> res(res_size);
	MPI_Recv(&res[0], res_size, MPI_DOUBLE, src_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	return res;
}


int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int matrix_size = atoi(argv[1]);

	srand(time(NULL));

	std::vector<double> matrix;

	std::vector<double> vec;

	std::vector<double> total_result;

	int rows_per_process = matrix_size;

	if (size > 1)
	{
		rows_per_process = matrix_size / size;
	}

	std::vector<double> result(rows_per_process);

	if (rank == 0) {

		matrix = make_matrix(matrix_size);

		vec = make_vector(matrix_size);

	}

	double start = MPI_Wtime();

	if (rank == 0) {
		
		send_str_and_vec(matrix, vec, size, matrix_size);

		for (int i = 0; i < rows_per_process; ++i) {
			for (int j = 0; j < matrix_size; ++j) {
				result[i] += matrix[i * matrix_size + j] * vec[j];
			}
		}

		total_result.insert(total_result.end(), result.begin(), result.end());

		for (int src_rank = 1; src_rank < size; ++src_rank) {

			double st = MPI_Wtime();
			result = recv_res(src_rank);
			double end = MPI_Wtime();
			communication += end - st;

			total_result.insert(total_result.end(), result.begin(), result.end());

		}
	}
	else {
		std::vector<double> column = recv_string();
		std::vector<double> vec = recv_vec();

		std::vector<double> result(column.size() / matrix_size);

		for (int i = 0; i < column.size() / matrix_size; ++i) {
			for (int j = 0; j < matrix_size; ++j) {
				result[i] += column[i * matrix_size + j] * vec[j];
			}
		}

		MPI_Send(&result[0], result.size(), MPI_DOUBLE, 0, 789, MPI_COMM_WORLD);

	}

	MPI_Barrier(MPI_COMM_WORLD);

	double end = MPI_Wtime();

	if (rank == 0)
	{
		double seconds = end - start;
		printf("The time: %f seconds\n", seconds);

		printf("The communication time: %f seconds\n", communication);

	}

	MPI_Finalize();

	return 0;
}