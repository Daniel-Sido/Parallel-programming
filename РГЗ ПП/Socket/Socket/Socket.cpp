#include "mpi.h"

int main(int argc, char** argv)
{
    
    socketRank = atoi(argv[1]);
    socketSize = atoi(argv[2]);
    int matrix_size = atoi(argv[3]);

    srand(time(NULL));

    communication = 0;
    
    std::vector<double> matrix(matrix_size * matrix_size);

    std::vector<double> vec(matrix_size);

    std::vector<double> total_result;

    int rows_per_process = matrix_size;

    int remaining_rows = 0;

    if (socketRank == socketSize - 1)
    {
        matrix = make_matrix(matrix_size);

        vec = make_vector(matrix_size);

    }

    if (socketSize > 1)
    {
        rows_per_process = matrix_size / socketSize;
        remaining_rows = matrix_size % socketSize;
        Init();
    }  

    std::vector<double> result(rows_per_process);

    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

    if (socketRank == socketSize - 1)
    {
      
        send_str_and_vec(matrix, vec, socketSize, matrix_size);
       
        for (int i = 0; i < rows_per_process; ++i) {
            for (int j = 0; j < matrix_size; ++j) {
                result[i] += matrix[i * matrix_size + j] * vec[j];
            }
        }

        total_result.insert(total_result.end(), result.begin(), result.end());
        
        for (int src_rank = 0; src_rank < socketSize - 1; ++src_rank) {

            if (src_rank == socketSize - 2)
                rows_per_process += remaining_rows;

            std::vector<double> result(rows_per_process);

            std::chrono::time_point<std::chrono::steady_clock> comm_start = std::chrono::steady_clock::now();
            MPI_MyRecv(result.data(), result.size(), "MPI_DOUBLE", src_rank);
            std::chrono::time_point<std::chrono::steady_clock> comm_end = std::chrono::steady_clock::now();

            communication += (comm_start - comm_end) / 1ms;

            total_result.insert(total_result.end(), result.begin(), result.end());

        }
    }
    else
    {
       
        if (socketRank == socketSize - 2)
                rows_per_process += remaining_rows;
        
        std::vector<double> column(rows_per_process * matrix_size);
        std::vector<double> result(rows_per_process);

        MPI_MyRecv(column.data(), column.size(), "MPI_DOUBLE", socketSize - 1);

        MPI_MyRecv(vec.data(), vec.size(), "MPI_DOUBLE", socketSize - 1);


        for (int i = 0; i < column.size() / matrix_size; ++i) {
            for (int j = 0; j < matrix_size; ++j) {
                result[i] += column[i * matrix_size + j] * vec[j];
            }
        }

        MPI_MySend(result.data(), result.size(), "MPI_DOUBLE", socketSize - 1);
    }


    std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();

    if (socketRank == socketSize - 1)
    {
        std::cout << "The time:" << (end - start) / 1ms << "ms\n";
        std::cout << "The communication time:" << communication <<" ms";

        /*std::cout << "result\n";

        for (auto& i : total_result)
        {
            std::cout << i << " ";
        }*/
    }
    else
    {
        Sleep(5000);
    }
    

    WSACleanup();

    return 0;
}