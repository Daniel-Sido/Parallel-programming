#ifndef MPI_H
#define MPI_H
#pragma comment (lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <sys/types.h>
#include <WinSock2.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <vector> 
#include <iostream> 
#include <string>
#include <cstdlib>
#include <chrono>

using namespace std::literals;

extern std::vector<SOCKET> sockets;
extern int socketRank;
extern int socketSize;
extern int communication;

extern  HOSTENT* hostent;

void Init();

void MPI_MySend(void* buf, int count, std::string type, int i); // Отправка сообщения (указатель на данные, размер данных, тип - int, куда отправить)

void MPI_MyRecv(void* buf, int count, std::string type, int i); // Приём сообщения (указатель на область памяти, размер получаемых данных, от какого процесса записывать)

std::vector<double> make_matrix(int size);

std::vector<double> make_vector(int size);

void send_str_and_vec(std::vector<double> matrix, std::vector<double> vec, int size, int matrix_size);

double fRand(double fMin, double fMax);
#endif