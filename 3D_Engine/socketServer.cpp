// C++ program to create Server
#include "socketServer.hpp"

#include <iostream>
#include <string.h>
#include <vector>
#include <winsock2.h>

// logging and measuring execution time
#include <chrono>
#include <iostream>
#include <fstream>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

static SafeQueue<ServerPacket>* serverTxQueuePtr;
static SafeQueue<ClientPacket>* serverRxQueuePtr;

static long int packet_count = 0;

void testlog()
{
	std::ofstream ofs("test.txt", std::ofstream::out);

	ofs << "lorem ipsum";

	ofs.close();
}


// Function that receive data
// from client
DWORD WINAPI serverReceive(LPVOID lpParam)
{
	// Created buffer[] to
	// receive message
	//char buffer[1024] = { 0 };

	struct ClientPacket client_packet;
	(void)memset(&client_packet, 0, sizeof(client_packet));
	char* buffer = (char*)&client_packet;

	// Created client socket
	SOCKET client = *(SOCKET*)lpParam;

	// Server executes continuously
	while (true) {

		// If received buffer gives
		// error then return -1
		(void)memset(&client_packet, 0, sizeof(client_packet));
		if (recv(client, buffer, sizeof(client_packet), 0)
			== SOCKET_ERROR) {
			cout << "recv function failed with error "
				<< WSAGetLastError() << endl;
			return -1;
		}

		serverRxQueuePtr->enqueue(client_packet);

		// If Client exits
		if (strcmp(buffer, "exit") == 0) {
			cout << "Client Disconnected."
				<< endl;
			break;
		}

		// Print the message
		// given by client that
		// was stored in buffer
		cout << "Client: " << client_packet.msg << endl;

		// Clear buffer message
		memset(buffer, 0,
			sizeof(buffer));
	}
	return 1;
}

// Function that sends data to client
DWORD WINAPI serverSend(LPVOID lpParam)
{
	static std::string log_file = "server_log.txt"; // C:/git/gump/3D_Engine/Images/
	std::ofstream server_log("server_log.txt", std::ofstream::out);
 
	struct ServerPacket packet;
	(void)memset(&packet, 0, sizeof(packet));
	
	// Created client socket
	SOCKET client = *(SOCKET*)lpParam;

	// Server executes continuously
	while (true) 
	{
		auto t1 = high_resolution_clock::now();
		packet = serverTxQueuePtr->dequeue();
		auto t2 = high_resolution_clock::now();

		// set the timestamp
		packet.timestamp = packet_count++;

		auto serialised_pkt = packet.serialize();
		auto t3 = high_resolution_clock::now();

		
		uint32_t syncByte = htonl(static_cast<uint32_t>(12345)); // Convert to network byte order
		if (send(client, reinterpret_cast<char*>(&syncByte), sizeof(syncByte), 0) == SOCKET_ERROR)
		{
			cout << "send failed with error "
				<< WSAGetLastError() << endl;
			continue;
		}
		auto t4 = high_resolution_clock::now();
		//// Send the size of the string first
		//uint32_t buffersize = htonl(serialised_pkt.size()); // Convert to network byte order
		//server_log  << " buffersize : " << buffersize
		//			<< " buffersize htonl: " << buffersize;
		//if(send(client, reinterpret_cast<char*>(&buffersize), sizeof(buffersize), 0) == SOCKET_ERROR)
		//{
		//	cout << "send failed with error "
		//		<< WSAGetLastError() << endl;
		//	continue;
		//}
		
		uint32_t buffersize = htonl(packet.bufferSize); // Convert to network byte order
		if (send(client, reinterpret_cast<char*>(&buffersize), sizeof(buffersize), 0) == SOCKET_ERROR)
		{
			cout << "send failed with error "
				<< WSAGetLastError() << endl;
			continue;
		}
		auto t5 = high_resolution_clock::now();
		// If sending failed, return -1
		if (send(client,
			reinterpret_cast<char*>(packet.buffer.data()),
			packet.buffer.size(), 0)
			== SOCKET_ERROR) 
		{
			cout << "send failed with error "
				<< WSAGetLastError() << endl;
			continue;
		}
		auto t6 = high_resolution_clock::now();

		server_log << "Execution time: \n Dequeue: " << duration_cast<milliseconds>(t2 - t1).count()
			<< "\n Serialize: " << duration_cast<milliseconds>(t3 - t2).count()
			<< "\n syncByte sending: " << duration_cast<milliseconds>(t4 - t3).count()
			<< "\n buffersize: " << duration_cast<milliseconds>(t5 - t4).count()
			<< "\n encoded image: " << duration_cast<milliseconds>(t6 - t4).count() << "\n\n";

	}
	server_log.close();
	return 1;
}

// Driver Code
int connection(SafeQueue<ServerPacket> * txqueue, SafeQueue<ClientPacket>* rxqueue)
{
	serverTxQueuePtr = txqueue;
	serverRxQueuePtr = rxqueue;
	// Data
	WSADATA WSAData;

	// Created socket server
	// and client
	SOCKET server, client;

	// Socket address for server
	// and client
	SOCKADDR_IN serverAddr, clientAddr;

	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// Making server
	server = socket(AF_INET,
		SOCK_STREAM, 0);

	// If invalid socket created,
	// return -1
	if (server == INVALID_SOCKET) {
		cout << "Socket creation failed with error:"
			<< WSAGetLastError() << endl;
		return -1;
	}
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5555);

	// If socket error occurred,
	// return -1
	if (::bind(server,
		(SOCKADDR*)&serverAddr,
		sizeof(serverAddr))
		== SOCKET_ERROR) {
		cout << "Bind function failed with error: "
			<< WSAGetLastError() << endl;
		return -1;
	}

	// Get the request from
	// server
	if (listen(server, 0)
		== SOCKET_ERROR) {
		cout << "Listen function failed with error:"
			<< WSAGetLastError() << endl;
		return -1;
	}

	cout << "Listening for incoming connections...." << endl;

	// Initialize client address
	int clientAddrSize = sizeof(clientAddr);

	// If connection established
	if ((client = accept(server,
		(SOCKADDR*)&clientAddr,
		&clientAddrSize))
		!= INVALID_SOCKET) {
		cout << "Client connected!" << endl;

		// Create variable of
		// type DWORD
		DWORD tid;

		// Create Thread t1
		HANDLE t1 = CreateThread(NULL,
			0,
			serverReceive,
			&client,
			0,
			&tid);

		// If created thread
		// is not created
		if (t1 == NULL) {
			cout << "Thread Creation Error: "
				<< WSAGetLastError() << endl;
		}

		// Create Thread t2
		HANDLE t2 = CreateThread(NULL,
			0,
			serverSend,
			&client,
			0,
			&tid);

		// If created thread
		// is not created
		if (t2 == NULL) {
			cout << "Thread Creation Error: "
				<< WSAGetLastError() << endl;
		}

		// Received Objects
		// from client
		WaitForSingleObject(t1,
			INFINITE);
		WaitForSingleObject(t2,
			INFINITE);

		// Close the socket
		closesocket(client);

		// If socket closing
		// failed.
		if (closesocket(server)
			== SOCKET_ERROR) {
			cout << "Close socket failed with error: "
				<< WSAGetLastError() << endl;
			return -1;
		}
		WSACleanup();
	}
	return 0;
}


void readImage()
{
	char filepath[] = "C:/Users/arun1/Desktop/Project3/3D_Engine/Images/test/1.png";

}
