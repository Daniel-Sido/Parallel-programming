#include "mpi.h"

std::vector<SOCKET> sockets;
int socketRank;
int socketSize;
HOSTENT* hostent;
int communication;

void Init()
{
    int start_port = 8080;
    WORD version = MAKEWORD(2, 2);
    WSADATA wsaData;
    typedef unsigned long IPNumber;
    
    WSAStartup(version, (LPWSADATA)&wsaData);
    std::vector<SOCKADDR_IN> servers(socketSize);
    
    sockets.resize(socketSize);
    //  Инициализация  сокетов 
    for (int i = 0; i < servers.size(); i++)
    {
        servers[i].sin_family = PF_INET;
        hostent = gethostbyname("localhost");
        servers[i].sin_addr.s_addr = (*reinterpret_cast<IPNumber*>(hostent->h_addr_list[0]));
        servers[i].sin_port = htons(start_port + i);
        sockets[i] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockets[i] == INVALID_SOCKET)
        {
            std::cout << "unable to create socket" << socketRank << std::endl;
            return;
        }
    }

    if (socketRank == socketSize - 1)
    {
        printf("Socket port: %d\n", servers[socketRank].sin_port);
        int retVal = ::bind(sockets[socketRank], (LPSOCKADDR) & (servers[socketRank]), sizeof(servers[socketRank]));
        if (retVal == SOCKET_ERROR)
        {
            printf("Unable to bind\n");
            int error = WSAGetLastError();
            printf("%d\n", error);
            WSACleanup();
            system("pause");
            return;
        }

        int task = 0;
        retVal = listen(sockets[socketRank], 10);
        if (retVal == SOCKET_ERROR)
        {
            printf("Unable to listen\n");
            int error = WSAGetLastError();
            printf("%d", error);
            system("pause");
            return;
        }
        SOCKADDR_IN from;
        int fromlen = sizeof(from);
        int buf = 0;

        int* temp = new int[1];
        buf = accept(sockets[socketRank], (struct sockaddr*)&from, &fromlen);
        retVal = recv(buf, (char*)temp, sizeof(int), 0);
        printf("Connect %d process \n", temp[0]);
        sockets[temp[0]] = buf;
    }

    for (int i = socketRank + 1; i < socketSize; i++)
    {
        int retVal = connect(sockets[i], (LPSOCKADDR)&servers[i], sizeof(servers[i]));
        if (retVal == SOCKET_ERROR)
        {
            std::cout << "unable to connect" << std::endl;
            int error = WSAGetLastError();
            printf("%ld", error);
            return;
        }

        int* temp = new int[1];
        temp[0] = socketRank;
        retVal = send(sockets[i], (char*)temp, sizeof(int), 0);
        
        if (retVal == SOCKET_ERROR)
        {
            std::cout << "unable to recv" << std::endl;
            int error = WSAGetLastError();
            printf("%d\n", error);
            return;
        }
    }

    int flag = socketSize - 1;
    int def = 1;
    if (socketRank == socketSize - 1)
        def++;

    for (int i = socketRank - def; i >= 0; i--)
    {
        if (socketRank < flag)
        {
            int retVal = ::bind(sockets[socketRank], (LPSOCKADDR) & (servers[socketRank]), sizeof(servers[socketRank]));
            if (retVal == SOCKET_ERROR)
            {
                printf("Unable to bind\n");
                int error = WSAGetLastError();
                printf("%d\n", error);
                WSACleanup();
                system("pause");
                return;
            }

            int task = 0;
            retVal = listen(sockets[socketRank], 10);
            if (retVal == SOCKET_ERROR)
            {
                printf("Unable to listen\n");
                int error = WSAGetLastError();
                printf("%d", error);
                system("pause");
                return;
            }
        }
        flag--;
        SOCKADDR_IN from;
        int fromlen = sizeof(from);
        int buf = 0;
        int* temp = new int[1];

        buf = accept(sockets[socketRank], (struct sockaddr*)&from, &fromlen);
        int retVal = recv(buf, (char*)temp, sizeof(int), 0);
        printf("Connect %d process \n", temp[0]);
        sockets[temp[0]] = buf;
    }
    int retVal = 0;
    std::cout << "Connection made sucessfully" << std::endl;
}

void MPI_MySend(void* buf, int count, std::string type, int i)
{
    int size_;
    if (type == "MPI_INT")
        size_ = count * sizeof(int);
    if (type == "MPI_DOUBLE")
        size_ = count * sizeof(double);

    if (send(sockets[i], (char*)buf, size_, 0) == SOCKET_ERROR)
    {
        std::cout << "unable to send" << std::endl;
        int error = WSAGetLastError();
        printf("%d\n", error);
        return;
    }
}

void MPI_MyRecv(void* buf, int count, std::string type, int i)
{
    int size_;
    if (type == "MPI_INT")
        size_ = count * sizeof(int);
    if (type == "MPI_DOUBLE")
        size_ = count * sizeof(double);

    if (recv(sockets[i], (char*)(buf), size_, 0) == SOCKET_ERROR)
    {
        std::cout << "unable to recv" << std::endl;
        int error = WSAGetLastError();
        printf("%d\n", error);
        return;
    }
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

    

    for (int dest_rank = 0; dest_rank < size - 1; dest_rank++) {

        if (dest_rank == size - 2)
        {
            current_rows += remaining_rows; 
        }

        std::chrono::time_point<std::chrono::steady_clock> communication_start = std::chrono::steady_clock::now();
        
        MPI_MySend(&matrix[start_row_index], current_rows * matrix_size, "MPI_DOUBLE", dest_rank);

        MPI_MySend(vec.data(), vec.size(), "MPI_DOUBLE", dest_rank);

        std::chrono::time_point<std::chrono::steady_clock> communication_end = std::chrono::steady_clock::now();

        communication += (communication_end - communication_start) / 1ms ;

        start_row_index += current_rows * matrix_size;
    }

    

}

double fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}