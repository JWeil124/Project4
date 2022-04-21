/*
 * File: sender.cpp
 *
 * Simple program that sends data on standard input to a remote host using the
 * RDT library.
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

int main(int argc, char** argv) {	
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " <remote host> <remote port>\n";
		exit(1);
	}

	int remote_port_num = std::stoi(argv[2]);

	// Create a reliable connection and connect to the specified remote host
	ReliableSocket socket;
	socket.connect_to_remote(argv[1], remote_port_num);

	// Create a char array and fill it with 0's
	std::array<char, ReliableSocket::MAX_DATA_SIZE> buff;
	buff.fill(0);

	auto start_time = std::chrono::system_clock::now();

	// Use stdin as the source for the data we will be sending
	int total_bytes = 0;
	int num_bytes_read = 0;
	while ((num_bytes_read = fread(buff.data(), 
									sizeof(char), 
									ReliableSocket::MAX_DATA_SIZE, 
									stdin))) {
		total_bytes += num_bytes_read;
		socket.send_data(buff.data(), num_bytes_read);
		cerr << "sender: sent " << num_bytes_read << " bytes of app data\n";
	}

	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end_time - start_time;

	cerr << "\nFinished sending, closing socket.\n";
	socket.close_connection();

	cerr << "\nSent " << total_bytes << " bytes in " 
			<< elapsed_seconds.count() << " seconds "
			<< "(" << total_bytes / elapsed_seconds.count() << " Bps)\n";

	cerr << "Estimated RTT:  " << socket.get_estimated_rtt() << " ms\n";

	return 0;
}
