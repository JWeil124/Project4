/*
 * File: ReliableSocket.cpp
 *
 * Reliable data transport (RDT) library implementation.
 *
 * Author(s):
 *
 */

// C++ library includes
#include <iostream>
#include <string.h>

// OS specific includes
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ReliableSocket.h"
#include "rdt_time.h"

using std::cerr;

/*
 * NOTE: Function header comments shouldn't go in this file: they should be put
 * in the ReliableSocket header file.
 */

ReliableSocket::ReliableSocket() {
	this->sequence_number = 0;
	this->expected_sequence_number = 0;
	this->estimated_rtt = 100;
	this->dev_rtt = 10;

	// TODO: If you create new fields in your class, they should be
	// initialized here.

	this->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (this->sock_fd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	this->state = INIT;
}

void ReliableSocket::accept_connection(int port_num) {
	if (this->state != INIT) {
		cerr << "Cannot call accept on used socket\n";
		exit(EXIT_FAILURE);
	}
	
	// Bind specified port num using our local IPv4 address.
	// This allows remote hosts to connect to a specific port.
	struct sockaddr_in addr; 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_num);
	addr.sin_addr.s_addr = INADDR_ANY;
	if ( bind(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) ) {
		perror("bind");
	}

	// Wait for a segment to come from a remote host
	char segment[MAX_SEG_SIZE];
	memset(segment, 0, MAX_SEG_SIZE);

	struct sockaddr_in fromaddr;
	unsigned int addrlen = sizeof(fromaddr);
	int recv_count = recvfrom(this->sock_fd, segment, MAX_SEG_SIZE, 0, 
								(struct sockaddr*)&fromaddr, &addrlen);		
	if (recv_count < 0) {
		perror("accept recvfrom");
		exit(EXIT_FAILURE);
	}

	/*
	 * UDP isn't connection-oriented, but calling connect here allows us to
	 * remember the remote host (stored in fromaddr).
	 * This means we can then use send and recv instead of the more complex
	 * sendto and recvfrom.
	 */
	if (connect(this->sock_fd, (struct sockaddr*)&fromaddr, addrlen)) {
		perror("accept connect");
		exit(EXIT_FAILURE);
	}

	// Check that segment was the right type of message, namely a RDT_CONN
	// message to indicate that the remote host wants to start a new
	// connection with us.
	RDTHeader* hdr = (RDTHeader*)segment;
	if (hdr->type != RDT_CONN) {
		cerr << "ERROR: Didn't get the expected RDT_CONN type.\n";
		exit(EXIT_FAILURE);
	}

	// TODO: You should implement a handshaking protocol to make sure that
	// both sides are correctly connected (e.g. what happens if the RDT_CONN
	// message from the other end gets lost?)
	// Note that this function is called by the connection receiver/listener.
	char send_data[1400] = {0};
	char recv_data[1400];


	hdr = (RDTHeader*)send_data;
	hdr->ack_number = htonl(0);
	hdr->sequence_number = htonl(0);
	hdr->type = RDT_SYNACK;

	while(true){
		this->reliable_send(send_data, sizeof(RDTHeader), recv_data);
		set_timeout_length(this->estimated_rtt + (4 * this->dev_rtt));
		hdr = (RDTHeader*)recv_data;

		if (hdr->type == RDT_ACK){ //Check for ACK
			break;
		}

		else{
			continue;
		}
	}

	this->state = ESTABLISHED;
	cerr << "INFO: Connection ESTABLISHED\n";
}

void ReliableSocket::connect_to_remote(char *hostname, int port_num) {
	if (this->state != INIT) {
		cerr << "Cannot call connect_to_remote on used socket\n";
		return;
	}
	
	// set up IPv4 address info with given hostname and port number
	struct sockaddr_in addr; 
	addr.sin_family = AF_INET; 	// use IPv4
	addr.sin_addr.s_addr = inet_addr(hostname);
	addr.sin_port = htons(port_num); 

	/*
	 * UDP isn't connection-oriented, but calling connect here allows us to
	 * remember the remote host (stored in fromaddr).
	 * This means we can then use send and recv instead of the more complex
	 * sendto and recvfrom.
	 */
	if(connect(this->sock_fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("connect");
	}
	
	char recv_data[1400];
	
	// Send an RDT_CONN message to remote host to initiate an RDT connection.
	char segment[sizeof(RDTHeader)];
	RDTHeader* hdr = (RDTHeader*)segment;

	hdr->ack_number = htonl(0);
	hdr->sequence_number = htonl(0);
	hdr->type = RDT_CONN;
	if (send(this->sock_fd, segment, sizeof(RDTHeader), 0) < 0) {
		perror("conn1 send");
	}

	// TODO: Again, you should implement a handshaking protocol for the
	// connection setup.
	// Note that this function is called by the connection initiator.
	
	////////*
	memset(hdr, 0, sizeof(RDTHeader));
	hdr = (RDTHeader*)recv_data;
	if (hdr->type != RDT_SYNACK){
		perror("The message was not a SYNACK");
	}

	memset(segment, 0, sizeof(RDTHeader));
	hdr = (RDTHeader*)segment;
	hdr->ack_number = htonl(0);
	hdr->sequence_number = htonl(0);
	hdr->type = RDT_ACK;
	this->timeout_send(segment);
	////////
	
	this->state = ESTABLISHED;
	cerr << "INFO: Connection ESTABLISHED\n";
}


// You should not modify this function in any way.
uint32_t ReliableSocket::get_estimated_rtt() {
	return this->estimated_rtt;
}

// You shouldn't need to modify this function in any way.
void ReliableSocket::set_timeout_length(uint32_t timeout_length_ms) {
	cerr << "INFO: Setting timeout to " << timeout_length_ms << " ms\n";
	struct timeval timeout;
	msec_to_timeval(timeout_length_ms, &timeout);

	if (setsockopt(this->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
					sizeof(struct timeval)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
}

void ReliableSocket::send_data(const void *data, int length) {
	if (this->state != ESTABLISHED) {
		cerr << "INFO: Cannot send: Connection not established.\n";
		return;
	}

 	// Create the segment, which contains a header followed by the data.
	char send_seg[MAX_SEG_SIZE] = {0};
	char recv_data[MAX_SEG_SIZE];


	// Fill in the header
	RDTHeader *hdr = (RDTHeader*)segment;
	hdr->sequence_number = htonl(sequence_number);
	hdr->ack_number = htonl(0);
	hdr->type = RDT_DATA;

	// Copy the user-supplied data to the spot right past the 
	// 	header (i.e. hdr+1).
	memcpy(hdr+1, data, length);
	
	if (send(this->sock_fd, segment, sizeof(RDTHeader)+length, 0) < 0) {
		perror("send_data send");
		exit(EXIT_FAILURE);
	}

	// TODO: This assumes a reliable network. You'll need to add code that
	// waits for an acknowledgment of the data you just sent, and keeps
	// resending until that ack comes.
	// Utilize the set_timeout_length function to make sure you timeout after
	// a certain amount of waiting (so you can try sending again).
	
	while(true){
		memset(recv_data, 0, MAX_SEG_SIZE);
		reliable_send(send_seg, sizeof(RDTHeader) + length, recv_data);

		hdr = (RDTHeader*)recv_data;
		if (hdr->type == RDT_ACK){
			if (this->sequence_number = ntohl(hdr->ack_number)){
				break;
			}
			else{
				continue;
			}
		else{
			continue;
		}
	this->sequence_number++;
}


int ReliableSocket::receive_data(char buffer[MAX_DATA_SIZE]) {
	//Ensures that timeout will not occur when data is received
	if (this->state != ESTABLISHED) {
		cerr << "INFO: Cannot receive: Connection not established.\n";
		return 0;
	}
	
	int recv_data_size = 0;
	while (true) {
		char recv_data[MAX_SEG_SIZE];
		char send_seg[sizeof(RDTHeader)] = {0};
		memset(recv_data, 0, MAX_SEG_SIZE);
		
		//Sets up header and data's pointers
		RDTHeader* hdr = (RDTHeader*)recv_data;
		void *data = (void*)(recv_data + sizeof(RDTHeader));
		
		//Receives data
		int recv_count = recv(this->sock_fd, recv_data, MAX_SEG_SIZE, 0);
		
		//Checks for error(timeout) and exits if there is one
		if (recv_count < 0) {
			perror("Error with receiving data. Terminating the program");
			exit(EXIT_FAILURE);
		}
		
		cerr << "Received segment,"
			<< "seq_num is: " << ntohl(hdr->sequence_number) << ", "
			<< "ack_num is: " << this->sequence_number << ", "
			<< "type is: " << hdr->type << "\n";
		
		uint32_t seqnum = hdr->sequence_number;
		
		if (hdr->type == RDT_ACK) {
			//Continues in the case of initial three way handshake
			continue;
		}
		if (hdr->type == RDT_CLOSE) {
			//Initiating sender
			hdr = (RDTHeader*)send_seg;
			hdr->sequence_number = htonl(0);
			hdr->ack_number = htonl(0);
			hdr->type = RDT_ACK;
			
			//Closing message
			this->timeout_send(send_seg);
			
			//
			this->state = FIN;
			break;
		}
		else {
			//Sends ACK for received data
			hdr = (RDTHeader*)send_seg;
			hdr->ack_number = seqnum;
			hdr->seqeuence_number = seqnum;
			hdr->type = RDT_ACK;
			if (send(this->sock_fd, send_seg, sizeof(RDTHeader), 0) < 0) {
				perror("Data sent");
			}
			if (nthol(seqnum) == this->sequence_number) {
				//Terminates loop
			}
			else {
				//Drops data instead of terminating the loop
				continue;
			}
		}


		recv_data_size = recv_count - sizeof(RDTHeader);
		this->sequence_number++;
		memcpy(buffer, data, recv_data_size);
		break;
		
	}

	return recv_data_size;
}


void ReliableSocket::close_connection() {
	// Construct a RDT_CLOSE message to indicate to the remote host that we
	// want to end this connection.
	char segment[sizeof(RDTHeader)];
	RDTHeader* hdr = (RDTHeader*)segment;

	hdr->sequence_number = htonl(0);
	hdr->ack_number = htonl(0);
	hdr->type = RDT_CLOSE;

	if (send(this->sock_fd, segment, sizeof(RDTHeader), 0) < 0) {
		perror("close send");
	}

	// TODO: As with creating a connection, you need to add some reliability
	// into closing the connection to make sure both sides know that the
	// connection has been closed.

	if (close(this->sock_fd) < 0) {
		perror("close_connection close");
	}
}
