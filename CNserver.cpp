#include <iostream>
#include <thread>
#include <vector>
#include <string.h>
#include <winsock.h>
#include <unordered_map>
#include <regex>
#include <array>
#include <sstream>

#define SERVER_PORT_NO 80
#define maxClient 80

using namespace std;

struct client_info {
	string name;
	u_short portnumbers = {};
	int fds = {};
	char* IPaddr = {};
};
string convert_client_info_to_string(client_info client) {
	ostringstream oss;
	oss << "/" << client.name << "/" << client.portnumbers << "/" << client.fds << "/" << client.IPaddr << "/";
	return oss.str();
}

//	
//void out_end(int connfd) {
//	char buffer[100];
//	string end = "e";
//	send(connfd, end.c_str(), end.length(), 0);
//	cout << "message send successfully" << endl;
//}
void in(int connfd, client_info client, unordered_map<string, vector<client_info>>& hash_table)
{
	// ----------------------------------------------------------------
	// recieves files from clients
	char file_buf[100] = { 0 };
	recv(connfd, file_buf, 100, 0);
	cout << "From Client: " << file_buf << endl;
	//-----------------------------------------------------------------
	
	string s = file_buf;
	regex regex_pattern("/");

	vector<string> out(
		sregex_token_iterator(s.begin(), s.end(), regex_pattern, -1),
		sregex_token_iterator()
	);
	array<string, 100> arr;
	int i = 0;
	for (auto& s : out) {
		arr[i] = s;
		i++;
	}
	client.name = arr[0];

	// Find the end of the filled part of the array.
	auto it = arr.begin() + i;

	for (auto it2 = arr.begin(); it2 != it; ++it2) {
		hash_table[*it2].push_back(client);
	}
	cout << endl;

	cout << "Files are avaiable in following clients: " << endl;
	for (const auto& element : hash_table)
	{
		cout << element.first << ": ";
		for (client_info value : element.second)
		{
			// server sends the main client_info back to the client
			if (client.name == element.first) {
				string s4 = convert_client_info_to_string(client);
				send(connfd, s4.c_str(), s4.length(), 0);
			}
			cout << value.name << " ";
		}
		cout << endl;
	}
	hash_table.erase(client.name);
	
	//----------------------------------------------------------------- 

	// recieves messages from clients
	while (1) {
		char buffer[100] = { 0 };
		char message[] = "request";

		recv(connfd, buffer, 100, 0);
		cout << "From Client (" << client.name << "):" << buffer << endl;
		if (!strcmp(buffer, "e")) break;

		if (strcmp(buffer, message)) {
			
			// send client_info struct
			//----------------------------------------------------------------------------

			string s2 = buffer;
			regex regex_pattern2(" ");

			vector<string> out(
				sregex_token_iterator(s2.begin(), s2.end(), regex_pattern2, -1),
				sregex_token_iterator()
			);
			array<string, 100> arr2;
			int i = 0;
			for (auto& s2 : out) {
				arr2[i] = s2;
				cout << arr2[i] << endl;
				i++;
			}

			string find_key = arr2[1];
			//----------------------------------------------------------------------------
			cout << "Following peers have your requested file: " << endl;

			// print the values of the key "abc"
			auto it = hash_table.find(find_key);
			if (it != hash_table.end()) {
				cout << find_key << ": ";
				for (client_info value : it->second) {

					string s3 = convert_client_info_to_string(value);
					if (client.name != value.name) {
						send(connfd, s3.c_str(), s3.length(), 0);
					}
					cout << value.name << " ";
				}
				cout << endl;
			}
		}
	}
	//out_end(connfd);
	cout << "Exits chat with client("<< client.name << ")" << endl;
}
void out(int connfd) {
	char buffer[100];
	send(connfd, "Connection Established! Hello\n", 31, 0);
}


int main() {
	int cfd[100];
	char* cIPaddr[100];
	int iterator = 0;

	unordered_map<string, vector<client_info>> hash_table;

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

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Socket Creation failed\n");
		return -1;
	}

	struct sockaddr_in addr;

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT_NO);
	
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("Bind failed on socket\n");
		return -1;
	}

	int backlog = 10;
	if (listen(fd, backlog) == -1) {
		perror("Listen Failed on server: \n");
		return -1;
	}

	int connfd;
	struct sockaddr_in cliaddr;
	client_info client;

	int cliaddr_len = sizeof(struct sockaddr);
	while (1) {
		connfd = accept(fd, (struct sockaddr*)&cliaddr, &cliaddr_len);
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

		if (connfd <= 0) {
			perror("accept failed on socket: \n");
		}

		thread T_out(out, connfd);
		thread T_in(in, connfd, client, ref(hash_table));
		T_out.detach();
		T_in.detach();
	}

	closesocket(fd);
	return 0;
}