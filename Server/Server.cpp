#include <iostream>
#include "vector"
#include <cstdlib>
#include <ctime>
#include "thread"
#include "chrono"
#include "math.h"
#include <winsock2.h>
using namespace std;
#pragma comment(lib, "ws2_32.lib") // link with ws2_32.lib


void calculate(int start, int actions, int** matrixA, int** matrixB, int** matrixRes, int size) {
	//this_thread::sleep_for(chrono::milliseconds(1000));
	if (start + actions >= size * size) {
		actions = size * size - start;
	}
	int startI, startJ;
	if (start < size) {
		startI = 0;
		startJ = start;
	}
	else {
		int row = start / size;
		startI = row;
		startJ = start - row * size;
	}
	while (actions > 0) {
		matrixRes[startI][startJ] = matrixA[startI][startJ] - matrixB[startI][startJ];
		if (startJ + 1 < size) {
			startJ++;
		}
		else {
			startI++;
			startJ = 0;
		}
		actions--;
	}

}

void handleUser(SOCKET* serverSocket, SOCKET* clientSocket) {
	int matrixSize;
	cout << "User handler entered..." << endl;
	int bytesReceived = recv(*clientSocket, (char*)&matrixSize, sizeof(matrixSize), 0);
	if (bytesReceived == SOCKET_ERROR) {
		std::cerr << "Error receiving data: " << WSAGetLastError() << "\n";
		closesocket(*clientSocket);
		closesocket(*serverSocket);
		WSACleanup();
		return;
	}
	cout << matrixSize << endl;
	int** matrixA = new int* [matrixSize];
	int** matrixB = new int* [matrixSize];
	int** matrixRes = new int* [matrixSize];
	cout << "Place for matrixes created." << endl;
	for (int i = 0; i < matrixSize; i++) {
		matrixRes[i] = new int[matrixSize];
		matrixA[i] = new int[matrixSize];
		matrixB[i] = new int[matrixSize];
		int bytesReceivedForRow = recv(*clientSocket, (char*)matrixA[i], sizeof(int) * matrixSize, 0);
		if (bytesReceivedForRow == SOCKET_ERROR) {
			std::cerr << "Error receiving data: " << WSAGetLastError() << "\n";
			closesocket(*clientSocket);
			closesocket(*serverSocket);
			WSACleanup();
			return;
		}
		int bytesReceivedForSecondRow = recv(*clientSocket, (char*)matrixB[i], sizeof(int) * matrixSize, 0);
		if (bytesReceivedForSecondRow == SOCKET_ERROR) {
			std::cerr << "Error receiving data: " << WSAGetLastError() << "\n";
			closesocket(*clientSocket);
			closesocket(*serverSocket);
			WSACleanup();
			return;
		}
	}
	cout << "Matrixes received." << endl;
	cout << endl;
	int numOfThreads;
	bytesReceived = recv(*clientSocket, (char*)&numOfThreads, sizeof(numOfThreads), 0);
	if (bytesReceived == SOCKET_ERROR) {
		std::cerr << "Error receiving data: " << WSAGetLastError() << "\n";
		closesocket(*clientSocket);
		closesocket(*serverSocket);
		WSACleanup();
		return;
	}
	cout << "Number of threads " << numOfThreads << ". Start performing..." << endl;
	int actionsPerThread = ceil(matrixSize * matrixSize / numOfThreads);
	vector<thread> threads;
	//count
	for (int i = 0; i < matrixSize * matrixSize; i += actionsPerThread) {
		threads.emplace_back(calculate, i, actionsPerThread, matrixA, matrixB, matrixRes, matrixSize);
	}
	for (int i = 0; i < threads.size(); i++) {
		threads[i].join();
	}
	cout << "Calculations finished. Sending result..." << endl;
	//send result
	for (int i = 0; i < matrixSize; i++) {
		int bytesSend = send(*clientSocket, (char*)matrixRes[i], sizeof(int) * matrixSize, 0);
		if (bytesSend == SOCKET_ERROR) {
			std::cerr << "Error receiving data: " << WSAGetLastError() << "\n";
			closesocket(*clientSocket);
			closesocket(*serverSocket);
			WSACleanup();
			return;
		}
	}
	cout << "Result succesfully sent." << endl;
	for (int i = 0; i < matrixSize; i++) {
		delete[] matrixA[i];
		delete[] matrixB[i];
		delete[] matrixRes[i];
	}
	delete[] matrixA;
	delete[] matrixB;
	delete[] matrixRes;
}

void serverWorks(SOCKET* serverSocket) {
	cout << "Server working. Waiting for the clieant..." << endl;
	while (true) {
		// listen for incoming connections
		if (listen(*serverSocket, SOMAXCONN) == SOCKET_ERROR) {
			std::cerr << "Error listening on socket: " << WSAGetLastError() << "\n";
			closesocket(*serverSocket);
			WSACleanup();
			return;
		}
		// accept incoming connections
		SOCKET clientSocket = accept(*serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Error accepting connection: " << WSAGetLastError() << "\n";
			closesocket(*serverSocket);
			WSACleanup();
			return;
		}
		cout << "Client connected." << endl;
		thread userHandler(handleUser, serverSocket, &clientSocket);
		userHandler.detach();
	}
}

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed.\n";
		return 1;
	}
	// create a socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
		WSACleanup();
		return 1;
	}
	// bind the socket to an address and port
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(8080);
	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << "Error binding socket: " << WSAGetLastError() << "\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}
	serverWorks(&serverSocket);
	// cleanup
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}