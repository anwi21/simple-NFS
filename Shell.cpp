// CPSC 3500: Shell
// Anwi Gundavarapu
// 6/1/25
// Implements a basic shell (command line interface) for the file system

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

using namespace std;

#include "Shell.h"
#include <string>

static const string PROMPT_STRING = "NFS> ";    // shell prompt

// Mount the network file system with server name and port number in the format of server:port
void Shell::mountNFS(string fs_loc) {
    size_t colon = fs_loc.find(':');
    if (colon == string::npos)
    {
      cerr <<  "Invalid server:port format" << endl;
      is_mounted = false;
      return;
    }
    string server = fs_loc.substr(0, colon);
    string port = fs_loc.substr(colon + 1);

    addrinfo hints{}, *addr_info;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    int status = getaddrinfo(server.c_str(), port.c_str(), &hints, &addr_info);
    if (status != 0)
    {
      cerr <<  "getaddrinfo error: " << gai_strerror(status) << endl;
      is_mounted = false;
      return;
    }

    //making sure socket is created
    //create the socket cs_sock and connect it to the server and port specified in fs_loc
    int cs_sock = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (cs_sock <0)
    {
      cerr <<  "socket error" << endl;
      is_mounted = false;
      freeaddrinfo(addr_info);
      return;
    }

    //making sure it is connected to the right port
    if (connect(cs_sock, addr_info->ai_addr, addr_info->ai_addrlen) < 0)
    {
      cerr <<  "connect error" << endl;
      is_mounted = false;
      freeaddrinfo(addr_info);
      close(cs_sock);
      return;
    }

    freeaddrinfo(addr_info);
    this->cs_sock = cs_sock;

    //if all the above operations are completed successfully, set is_mounted to true
    is_mounted = true;


}

// Unmount the network file system if it was mounted
void Shell::unmountNFS() {
    // close the socket if it was mounted
    if (is_mounted)
    {
      close(cs_sock);
      is_mounted = false;
    }
}

// Remote procedure call on mkdir
void Shell::mkdir_rpc(string dname)
{
  //setting message format and sending request to server
  string request = "mkdir " + dname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving message from server and making sure it is properly recieved
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //preparing to recieve body of display
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  //recieving body properly
  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }
  //calling helper to display method
  print_response(header, body);
}

// Remote procedure call on cd
void Shell::cd_rpc(string dname) {

  //setting request format and sending request
  string request = "cd " + dname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //caling helper to display response
  print_response(header, body);
}

// Remote procedure call on home
void Shell::home_rpc() {

  //setting request format and sending request
  string request = "home\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }
  //caling helper to display response
  print_response(header, body);
}

// Remote procedure call on rmdir
void Shell::rmdir_rpc(string dname) {

  //setting request format and sending request
  string request = "rmdir " + dname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //setting request format and sending request
  //recieving response header --> error or success
  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }
  //caling helper to display response
  print_response(header, body);
}

// Remote procedure call on ls
void Shell::ls_rpc() {
  //setting request format and sending request
  string request = "ls\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //checking if body  is empty,
  //if file is empty output: empty file
  //else print file data
  if (body.empty()) {
    cout << "empty file" << endl;
  } else {
    cout << body;
    if (body.back() != '\n') cout << endl;
  }

}

// Remote procedure call on create
void Shell::create_rpc(string fname) {
  //setting request format and sending request
  string request = "create " + fname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }


  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //caling helper to display response
  print_response(header, body);
}

// Remote procedure call on append
void Shell::append_rpc(string fname, string data) {
  //setting request format and sending request
  string request = "append " + fname + " " + data + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }


  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //caling helper to display response
  print_response(header, body);
}

// Remote procesure call on cat
void Shell::cat_rpc(string fname) {

  //setting request format and sending request
  string request = "cat " + fname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //checking if body  is empty,
  //if file is empty output: empty file
  //else print file data
  if (body.empty()) {
    cout << "empty file" << endl;
  } else {
    cout << body;
    if (body.back() != '\n') cout << endl;
  }

}

// Remote procedure call on head
void Shell::head_rpc(string fname, int n) {
  //setting request format and sending request
  string request = "head " + fname + " " + to_string(n) +"\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //display n bytes if succesfull, error message if not
  size_t status_end = header.find("\r\n");
  string status_line = header.substr(0, status_end);
  int status_code = stoi(status_line.substr(0, 3));
  string status_msg = status_line.substr(4);

  if (status_code == 200 && status_msg == "OK") {
    if (!body.empty())
      cout << body << endl;
  } else {
    cout << status_code << " " << status_msg << endl;
  }

}

// Remote procedure call on rm
void Shell::rm_rpc(string fname) {
  //setting request format and sending request
  string request = "rm " + fname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //caling helper to display response
  print_response(header, body);
}

// Remote procedure call on stat
void Shell::stat_rpc(string fname) {

  //setting request format and sending request
  string request = "stat " + fname + "\r\n";
  size_t sent = 0;
  while (sent < request.size())
  {
    ssize_t n = send(cs_sock, request.data() + sent, request.size() - sent, 0);
    if (n < 0)
    {
      perror("send error");
      return;
    }
    sent += n;
  }

  //recieving response header --> error or success
  string header;
  char ch;
  while (header.find("\r\n\r\n") == string::npos)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    header += ch;
  }

  //recieving response body and storing it
  size_t len_position = header.find( "Length:" );
  size_t rn_position = header.find( "\r\n", len_position );
  int body_len = 0;
  if (len_position != string::npos && rn_position != string::npos)
  {
    body_len = stoi(header.substr(len_position +
      7, rn_position - (len_position + 7)));
  }

  string body;
  while ((int)body.size() < body_len)
  {
    ssize_t n = recv(cs_sock, &ch, 1, 0);
    if (n < 0)
    {
      perror("recv error");
      return;
    }
    body += ch;
  }

  //cannot call helper so stats are displayed properly
  size_t status_end = header.find("\r\n");

  //storing header data to appropriately output message
  string status_line = header.substr(0, status_end);
  int status_code = stoi(status_line.substr(0, 3));
  string status_msg = status_line.substr(4); // Skip "### "

  // if the message is 200 OK, stats succesful: display stats
  if (status_code == 200 && status_msg == "OK") {
    if (!body.empty()) {
      cout << body;  // body already has newline if needed
      if (body.back() != '\n') cout << endl;
    }
  } else {
    // Not successful -- error code printed --> cant get stats
    cout << status_code << " " << status_msg << endl;
  }
}


//helper function to appropriately display messages
void Shell::print_response(const string& header, const string& body) {

  //if the display message is not proper
  size_t status_end = header.find("\r\n");
  if (status_end == string::npos) {
    cout << "Invalid response\n";
    return;
  }

  //storing header data to appropriately output message
  string status_line = header.substr(0, status_end);
  int status_code = stoi(status_line.substr(0, 3));
  string status_msg = status_line.substr(4); // Skip "### "

  // if the message is 200 OK, request was succesful,
  if (status_code == 200 && status_msg == "OK") {
    cout << "success" << endl;
  } else {
    // Not successful -- error code printed
    cout << status_code << " " << status_msg << endl;
  }

  //printing body if not empty
  if (!body.empty()) {
    cout << body;  // body already has newline if needed
    if (body.back() != '\n') cout << endl;
  }
}

// Executes the shell until the user quits.
void Shell::run()
{
  // make sure that the file system is mounted
  if (!is_mounted)
    return;

  // continue until the user quits
  bool user_quit = false;
  while (!user_quit) {

    // print prompt and get command line
    string command_str;
    cout << PROMPT_STRING;
    getline(cin, command_str);

    // execute the command
    user_quit = execute_command(command_str);
  }

  // unmount the file system
  unmountNFS();
}

// Execute a script.
void Shell::run_script(char *file_name)
{
  // make sure that the file system is mounted
  if (!is_mounted)
    return;
  // open script file
  ifstream infile;
  infile.open(file_name);
  if (infile.fail()) {
    cerr << "Could not open script file" << endl;
    return;
  }


  // execute each line in the script
  bool user_quit = false;
  string command_str;
  getline(infile, command_str, '\n');
  while (!infile.eof() && !user_quit) {
    cout << PROMPT_STRING << command_str << endl;
    user_quit = execute_command(command_str);
    getline(infile, command_str);
  }

  // clean up
  unmountNFS();
  infile.close();
}


// Executes the command. Returns true for quit and false otherwise.
bool Shell::execute_command(string command_str)
{
  // parse the command line
  struct Command command = parse_command(command_str);

  // look for the matching command
  if (command.name == "") {
    return false;
  }
  else if (command.name == "mkdir") {
    mkdir_rpc(command.file_name);
  }
  else if (command.name == "cd") {
    cd_rpc(command.file_name);
  }
  else if (command.name == "home") {
    home_rpc();
  }
  else if (command.name == "rmdir") {
    rmdir_rpc(command.file_name);
  }
  else if (command.name == "ls") {
    ls_rpc();
  }
  else if (command.name == "create") {
    create_rpc(command.file_name);
  }
  else if (command.name == "append") {
    append_rpc(command.file_name, command.append_data);
  }
  else if (command.name == "cat") {
    cat_rpc(command.file_name);
  }
  else if (command.name == "head") {
    errno = 0;
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) {
      head_rpc(command.file_name, n);
    } else {
      cerr << "Invalid command line: " << command.append_data;
      cerr << " is not a valid number of bytes" << endl;
      return false;
    }
  }
  else if (command.name == "rm") {
    rm_rpc(command.file_name);
  }
  else if (command.name == "stat") {
    stat_rpc(command.file_name);
  }
  else if (command.name == "quit") {
    return true;
  }

  return false;
}

// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
Shell::Command Shell::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct Command empty = {"", "", ""};

  // grab each of the tokens (if they exist)
  struct Command command;
  istringstream ss(command_str);
  int num_tokens = 0;
  if (ss >> command.name) {
    num_tokens++;
    if (ss >> command.file_name) {
      num_tokens++;
      if (ss >> command.append_data) {
        num_tokens++;
        string junk;
        if (ss >> junk) {
          num_tokens++;
        }
      }
    }
  }

  // Check for empty command line
  if (num_tokens == 0) {
    return empty;
  }

  // Check for invalid command lines
  if (command.name == "ls" ||
      command.name == "home" ||
      command.name == "quit")
  {
    if (num_tokens != 1) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "mkdir" ||
      command.name == "cd"    ||
      command.name == "rmdir" ||
      command.name == "create"||
      command.name == "cat"   ||
      command.name == "rm"    ||
      command.name == "stat")
  {
    if (num_tokens != 2) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "append" || command.name == "head")
  {
    if (num_tokens != 3) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else {
    cerr << "Invalid command line: " << command.name;
    cerr << " is not a command" << endl;
    return empty;
  }

  return command;
}
