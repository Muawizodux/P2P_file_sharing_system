#include <iostream>
#include <winsock.h>
#include <string>
#include <string.h>
#include <filesystem>
#include <sstream>
#include <vector>
#include <array>
#include <thread>
#include <fstream>
#include <chrono>



namespace fs = std::filesystem;
using namespace std;


#define SERVER_PORT_NO 80
bool out_sleep_FLAG;      // makes out() thread sleep temporarily
bool should_exit_FLAG;   // makes in() and out() thread sleep permenantly
string demanded_file;


struct client_info {
	string name;  // dell
	u_short portnumbers = {};
	int fds = {};
	string IPaddr = {};
	vector<client_info> client_vec = {}; // define a vector of client_info as a member variable

	void display_client_info()
	{
		cout << endl << "Client Name: " << name;
		cout << endl << "Client IP address: " << IPaddr;
		cout << endl << "Client PORT: " << portnumbers;
		cout << endl << "Client fd: " << fds << endl;
	}
	// define a member function to add client_info to the vector
	void add_client_info(client_info& cli) {
		client_vec.push_back(cli);
	}
	//Member function to check if client_info is already in the vector
	bool is_already_present(client_info cli) {
		for (auto check : client_vec) {
			// dell == dell
			if (check.name == cli.name && check.portnumbers == cli.portnumbers) {
				return 1;
			}
		}
		return 0;
	}

};
client_info convert_string_to_client_info(string str) {
	client_info client;
	istringstream iss(str);

	iss.ignore();
	getline(iss, client.name, '/');
	iss >> client.portnumbers; iss.ignore();
	iss >> client.fds; iss.ignore();
	getline(iss, client.IPaddr, '/');

	return client;
}


void file_upload(int connfd) {/*
	char buffer[500] = { 0 };
	recv(connfd, buffer, 500, 0);*/

	string path = ".\\files\\";
	//string filename(buffer);
	string file_pathname = path + demanded_file;

	// Check if file exists and send appropriate response to client
	ifstream in_file(file_pathname, ios::binary);
	if (in_file.good()) {
		send(connfd, "ok", 2, 0);
		char buffer[1024];
		int read_bytes = 0;
		while ((read_bytes = in_file.read(buffer, 1024).gcount()) > 0) {
			send(connfd, buffer, read_bytes, 0);
		}
		in_file.close();
	}
	else {
		send(connfd, "error", 5, 0);
	}
}
void file_download(int connfd) {
	//send(connfd, demanded_file.c_str(), demanded_file.length(), 0);
	// 
	// Receive file from client
	char buffer[1024];
	ofstream out_file;
	out_file.open(demanded_file, ios::binary);
	int received_bytes = 0;
	while ((received_bytes = recv(connfd, buffer, 1024, 0)) > 0) {
		out_file.write(buffer, received_bytes);
	}
	out_file.close();
}

//
//void peer_out(int connfd) {
//	// for sending message to the other client
//	while (1) {
//		string message;
//		getline(cin, message);
//		send(connfd, message.c_str(), message.length(), 0);
//
//		cout << "Message to peer: " << message << endl;
//		if (!strcmp(message.c_str(), "e")) break;
//	}
//}
//void peer_in(int connfd) {
//	// for recieving message from the other client
//	while (1) {
//		char buffer[100] = {};
//		recv(connfd, buffer, 100, 0);
//		buffer[99] = '\0';
//		cout << "Message from peer: " << buffer << endl;
//	}
//}
void peer_out(int connfd) {
	// for sending message to the other client
	while (1) {
		string message;
		getline(cin, message);

		// Process file transfer commands
		if (message == "upload") {
			cout << "Enter the file name to upload: ";
			getline(cin, demanded_file);
			string request = "upload " + demanded_file;
			send(connfd, request.c_str(), request.length(), 0);
			char buffer[500] = { 0 };
			int received_bytes = recv(connfd, buffer, 500, 0);
			string response(buffer, received_bytes);
			if (response == "ok") {
				file_upload(connfd);
			}
			else {
				cout << "Error uploading file." << endl;
			}
		}
		else if (message == "download") {
			cout << "Enter the file name to download: ";
			getline(cin, demanded_file);
			string request = "download " + demanded_file;
			send(connfd, request.c_str(), request.length(), 0);
			char buffer[500] = { 0 };
			int received_bytes = recv(connfd, buffer, 500, 0);
			string response(buffer, received_bytes);
			if (response == "ok") {
				file_download(connfd);
			}
			else {
				cout << "Error downloading file." << endl;
			}
		}
		else {
			// Send message to the other client
			send(connfd, message.c_str(), message.length(), 0);
			cout << "Message to peer: " << message << endl;
			if (message == "exit") {
				should_exit_FLAG = 1;
				out_sleep_FLAG = 1;
				return;
			}
		}
	}
}


void peer_in(int connfd) {
	// for receiving message from the other client
	while (1) {
		char buffer[500] = { 0 };
		int received_bytes = recv(connfd, buffer, 500, 0);
		string message(buffer, received_bytes);

		// Process file transfer commands
		if (message.substr(0, 7) == "upload ") {
			string file_name = message.substr(7);
			send(connfd, "ok", 2, 0);
			char buffer[1024];
			ofstream out_file;
			out_file.open(file_name, ios::binary);
			int received_bytes = 0;
			while ((received_bytes = recv(connfd, buffer, 1024, 0)) > 0) {
				out_file.write(buffer, received_bytes);
			}
			out_file.close();
			cout << "File " << file_name << " received from peer." << endl;
		}
		else if (message.substr(0, 9) == "download ") {
			string file_name = message.substr(9);
			// Check if file exists and send appropriate response to client
			ifstream in_file(file_name, ios::binary);
			if (in_file.good()) {
				send(connfd, "ok", 2, 0);
				char buffer[1024];
				int read_bytes = 0;
				while ((read_bytes = in_file.read(buffer, 1024).gcount()) > 0) {
					send(connfd, buffer, read_bytes, 0);
				}
				in_file.close();
			}
			else {
				send(connfd, "error", 5, 0);
			}
		}
		else {
			// Print message from the other client
			cout << "Message from peer: " << message << endl;
			if (message == "exit") {
				should_exit_FLAG = 1;
				break;
			}
		}
	}
}




void connecting_peers(int fd) {
	cout << "This peer is listening..." << endl;
	int cfd[100];
	char* cIPaddr[100];
	int iterator = 0;

	int connfd;
	struct sockaddr_in cliaddr;
	client_info client;

	int cliaddr_len = sizeof(struct sockaddr);
	while (1) {
		connfd = accept(fd, (struct sockaddr*)&cliaddr, &cliaddr_len);
		if (connfd < 0) {
			perror("accept failed on socket: \n");
			exit(EXIT_FAILURE);
		}
		char* IPaddr = inet_ntoa(cliaddr.sin_addr);

		cout << "Client address : " << IPaddr << "\n";
		cout << "client port is : " << cliaddr.sin_port << endl;
		cfd[iterator] = cliaddr.sin_port;
		cIPaddr[iterator] = IPaddr;
		iterator++;
		cout << cIPaddr[0] << "\n";

		//-------------------------------------------------
		client.IPaddr = IPaddr;
		client.portnumbers = cliaddr.sin_port;
		client.fds = connfd;
		//-------------------------------------------------

		thread P_in(peer_in, connfd);
		P_in.detach();

		out_sleep_FLAG = true;
		bool decision;
		cout << "Do you want to chat with the client: [Yes/No]" << endl;
		cin >> decision;
		// send the `stop` call to the server - server stops for 10 seconds
		if (decision = true) {

			thread P_out(peer_out, connfd);
			P_out.detach();

		}
		else {
			cout << "File is downloading..." << endl;
		}
	}


}
void connect_peers(client_info client_device, client_info main_client) {

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Socket Creation failed\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr;

	cout << "------------------------------------------" << endl;
	client_device.display_client_info();
	cout << "------------------------------------------" << endl;

	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // (client_device.IPaddr).c_str()
	addr.sin_family = AF_INET;
	addr.sin_port = htons(client_device.portnumbers); // destination port number
	memset(&(addr.sin_zero), 0, 8);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("Socket Connect failed\n");
		exit(EXIT_FAILURE);
	}

	//out_sleep_FLAG = true;
	bool decision;
	cout << "Do you want to chat with the client: [Yes = 1 /No = 0]" << endl;
	cin >> decision;
	// send the `stop` call to the server - server stops for 10 seconds
	if (decision = true) {
		thread P_in(peer_in, fd);
		thread P_out(peer_out, fd);

		P_in.detach();
		P_out.detach();

	}
	else {
		cout << "File is downloading..." << endl;
		// insert file downloading mechanism here
	}
}


void in(int connfd, string name)
{
	// for recieving message from the server

	client_info main_client; // GLOBAL
	client_info client_data; // GLOBAL

	should_exit_FLAG = false;
	while (1 && !should_exit_FLAG) {
		client_info temp_client;
		char buffer[100];
		char message[] = "/";
		string message2 = "/" + name;

		string choice;
		vector<string> devices = {};

		recv(connfd, buffer, 100, 0);
		cout << "From Server: " << buffer << endl;
		if (!strcmp(buffer, "e")) break;

		// for other client_info
		if (buffer[0] == message[0] && buffer[1] != message2[1]) {

			temp_client = convert_string_to_client_info(buffer);
			devices.push_back(temp_client.name);
			if (client_data.is_already_present(temp_client)) {
				cout << "ERROR: Client " << temp_client.name << " info is already present!" << endl;
			}
			else {
				client_data.add_client_info(temp_client); // client_vec : hp, dell, mac
				for (auto cli : client_data.client_vec) {
					cli.display_client_info();
				}
			}

			out_sleep_FLAG = true;

			cout << "Which client do you want to connect to:" << endl;
			for (auto cli : devices) {
				cout << cli << endl;
			}
			getline(cin, choice);
			for (auto cli : client_data.client_vec) {
				if (choice == cli.name) {
					out_sleep_FLAG = false;
					connect_peers(cli, main_client);
				}
			}
		}
		// for main client_info - only executes 1 time
		if (buffer[0] == message2[0] && buffer[1] == message2[1] && buffer[2] == message2[2]) { // -->    /hp/127.0.0.1/PORT_number/FD
			main_client = convert_string_to_client_info(buffer);
			main_client.display_client_info();
			// -------------------------------------- CLIENT GOES TO LISTEN STATE -------------------------------------------------------

			int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd == -1) {
				perror("Socket Creation failed\n");
				//return nullptr;
			}

			struct sockaddr_in addr;

			addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // main_client.IPaddr.c_str()
			addr.sin_family = AF_INET;
			addr.sin_port = htons(main_client.portnumbers); // enter main_client client_info.portnumber!

			if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
				perror("Bind failed on socket\n");
				//return -1;

			}

			int backlog = 10;
			if (listen(fd, backlog) == -1) {
				perror("Listen Failed on server: \n");
				//	return -1;
			}

			thread PeerConnect(connecting_peers, fd);
			PeerConnect.detach();
			// -----------------------------------------------------------------------------------------------------------------
		}
	}
}
void out(int connfd, string file_buffer) {
	// for recieving message to the server

	// ------------------------------------------------------------
	// converts string to const char*
	const char* file_buf;
	string sym(file_buffer); // sym = file_buffer
	file_buf = sym.c_str();

	// send file buffer
	send(connfd, file_buf, strlen(file_buf), 0);
	//---------------------------------------------------------------

	// send message to server
	out_sleep_FLAG = false;
	while (1 && !should_exit_FLAG) {
		//start:
		if (out_sleep_FLAG) {
			this_thread::sleep_for(chrono::seconds(20));
			cout << "Done" << endl;
			out_sleep_FLAG = false;
		}
		else {
			char buffer[100];
			string key, filename;

			cin.getline(buffer, 100);
			send(connfd, buffer, strlen(buffer), 0);
			cout << "To Server: " << buffer << endl;
			int i = 0;
			for (;buffer[i] != ' '; i++) {
				key.push_back(buffer[i]);
			}
			i++;
			for (;buffer[i] != '\0'; i++) {
				filename.push_back(buffer[i]);
			}
			if (key == "request") {
				demanded_file = filename;
			}
			//cout << demanded_file << endl;
			if (!strcmp(buffer, "e"))break;
		}
	}
}


int main() {

	string path = "/Users/HP/source/repos/CNclient/files/";
	char files[100] = { 0 };
	string sample, name;

	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws) < 0)
	{
		cout << "WSA failed to initialize\n";
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	else {
		cout << "Socket initialized\n";
	}

	cout << "Enter the name of your device: ";
	getline(cin, name);

	// creating a socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Socket Creation failed\n");
		return -1;
	}

	struct sockaddr_in sa_addr;

	// binding a socket
	sa_addr.sin_family = AF_INET;
	sa_addr.sin_port = htons(80);
	sa_addr.sin_addr.s_addr = inet_addr("127.0.0.1");	// INADDR_ANY, listens on anny IP		//Assigning local address of the machine to the server
	memset(&(sa_addr.sin_zero), 0, 8);					//8 is size of sin_zero and 0 is the value that we are setting to it


	if (connect(fd, (struct sockaddr*)&sa_addr, sizeof(sa_addr)) == -1) {
		perror("Socket Connect failed\n");
		return -1;
	}
	cout << endl;

	//-----------------------------------------------------------------------------
	string file;
	cout << "Available files on this " + name + ":" << endl;
	for (const auto& entry : fs::directory_iterator(path))
	{
		cout << entry.path().filename() << endl;
		sample += entry.path().filename().string();

		sample += '/';

	}
	file = name + '/' + sample;
	//-----------------------------------------------------------------------------

	thread T_out(out, fd, file);
	thread T_in(in, fd, name);

	T_out.join();
	T_in.join();

	closesocket(fd);
	return 0;
}