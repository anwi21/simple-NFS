// CPSC 3500: File System
// Anwi Gundavarapu
// 6/1/25

#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>

#include "FileSys.h"
using namespace std;

int main(int argc, char* argv[])
{
    const int BACKLOG = 5;

    if (argc < 2) {
        cout << "Usage: ./nfsserver port#\n";
        return -1;
    }
    int port = atoi(argv[1]);
    //getting port#

    //networking part: create the socket and accept the client connection
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); //change this line when necessary!
    if (socket_fd < 0)
    {
        perror("Error on socket");
        return -1;
    }

    //establashing server address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    //for TCP byte order
    server_addr.sin_port = htons(port);

    //to make sure port can be used even server clpsed accidentally
    int yes = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


    //bind
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error on bind");
        close(socket_fd);
        return -1;
    }

    //listen
    if (listen(socket_fd, BACKLOG) < 0)
    {
        perror("Error on listen");
        close(socket_fd);
        return -1;
    }

    //accept client request
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int new_socket_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (new_socket_fd < 0)
    {
        perror("Error on accept");
        close(socket_fd);
        return -1;
    }

    //close the listening socket
    close(socket_fd);

    //mount the file system
    FileSys fs;
    fs.mount(new_socket_fd); //assume that sock is the new socket created
    //for a TCP connection between the client and the server.

    //loop: get the command from the client and invoke the file
    //system operation which returns the results or error messages back to the clinet
    //until the client closes the TCP connection.
    bool done = false;
    string cmdLine;
    while (!done)
    {
        cmdLine.clear();
        char ch;
        while (true)
        {
            //making sure request is received
            int n = recv(new_socket_fd, &ch, 1, 0);
            if (n == 0)
            {
                perror("Connection closed");
                done = true;
                break;
            };

            if (n < 0)
            {
                perror("Error on recv");
                done = true;
                break;
            }
            cmdLine += ch;

            if (cmdLine.size() >= 2 && cmdLine.substr(cmdLine.size()-2) == "\r\n")
            {
                break;
            }
        }


        if (done)
        {
            break;
        }

        //removing traling space
        if (cmdLine.size()>= 2)
        {
            cmdLine = cmdLine.substr(0,cmdLine.size()-2);
        }


        //parsing command line input to args and commands
        istringstream inputString(cmdLine);
        string command, arg1, arg2;
        inputString >> command >> arg1;
        getline(inputString, arg2);

        //removing leading spcae
        if (!arg2.empty() && arg2[0] == ' ')
        {
            arg2 = arg2.substr(1);
        }

        //calling fs command based on command input
        //also input appropraite args for each fs call
        if (command == "mkdir")
        {
            fs.mkdir(arg1.c_str());
        }
        else if (command == "cd")
        {
            fs.cd(arg1.c_str());
        }
        else if (command == "home")
        {
            fs.home();
        }
        else if (command == "rmdir")
        {
            fs.rmdir(arg1.c_str());
        }
        else if (command == "ls")
        {
            fs.ls();
        }
        else if (command == "create")
        {
            fs.create(arg1.c_str());
        }
        else if (command == "append")
        {
            fs.append(arg1.c_str(), arg2.c_str());
        }
        else if (command == "cat")
        {
            fs.cat(arg1.c_str());
        }
        else if (command == "head")
        {
            unsigned int n = stoi(arg2);
            fs.head(arg1.c_str(), n);
        }
        else if (command == "rm")
        {
            fs.rm(arg1.c_str());
        }
        else if (command == "stat")
        {
            fs.stat(arg1.c_str());
        }
        else
        {
            string error = "500 Unknown command\r\nLength:0\r\n\r\n";
            write(new_socket_fd, error.c_str(), error.size());
            continue;
        }

        //sending response to client
        const string &message = fs.response;
        size_t total_sent = 0;
        while (total_sent < message.size())
        {
            ssize_t sent = send(new_socket_fd, message.c_str() + total_sent, message.size() - total_sent,0);

            if (sent == 0)
            {
                perror("Connection closed");
                done = true;
                break;
            }
            if (sent < 0)
            {
                perror("Error on send");
                done = true;
                break;
            }
            total_sent += sent;
        }

    }


    //unmout the file system
    fs.unmount();
    //close communciation socket
    close(new_socket_fd);
    return 0;
}

