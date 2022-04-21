/*
 * File: receiver.cpp
 *
 * Simple program that receives data from a remote host using the
 * RDT library, writing the received data to standard output.
 * 
 * You should NOT modify this file.
 */

// C++ standard libraries
#include <string>
#include <chrono>
#include <iostream>
#include <array>

// RDT library
#include "ReliableSocket.h"

using std::cerr;

int main(int argc, char **argv) {	
	if (argc != 2) { 
		cerr << "Usage: " << argv[0] << " <listening port>\n";
		exit(1);
	}

	ReliableSocket socket;
	socket.accept_connection(std::stoi(argv[1]));

	auto start_time = std::chrono::system_clock::now();
	std::array<char, ReliableSocket::MAX_DATA_SIZE> segment;
	int bytes_received = socket.receive_data(segment.data());		

	// Keep receiving data until we do a receive that gives us 0 bytes.
	int total_bytes = 0;
	while (bytes_received != 0) {
		cerr << "receiver: received " << bytes_received << " bytes of app data\n";
		total_bytes += bytes_received;

		// write received data to stdout
		fwrite(segment.data(), sizeof(char), bytes_received, stdout);
		fflush(stdout);
		bytes_received = socket.receive_data(segment.data());		
	}

	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end_time - start_time;

	cerr << "\nReceived " << total_bytes << " bytes in " 
			<< elapsed_seconds.count() << " seconds "
			<< "(" << total_bytes / elapsed_seconds.count() << " Bps)\n";

	cerr << "\nFinished receiving file, closing socket.\n";
	socket.close_connection();

	fflush(stdout);
}
