// CPSC 3500: File System
// Anwi Gundavarapu
// 6/1/25
// Implements the file system commands that are available to the shell.

#ifndef FILESYS_H
#define FILESYS_H

#include "BasicFileSys.h"
#include "Blocks.h"
#include <string>

using namespace std;

class FileSys {

  public:

    //this is the only way I can send the message
    //without using freidn construct
    string response;

    // mounts the file system
    void mount(int sock);

    // unmounts the file system
    void unmount();

    // make a directory
    void mkdir(const char *name);

    // switch to a directory
    void cd(const char *name);

    // switch to home directory
    void home();

    // remove a directory
    void rmdir(const char *name);

    // list the contents of current directory
    void ls();

    // create an empty data file
    void create(const char *name);

    // append data to a data file
    void append(const char *name, const char *data);

    // display the contents of a data file
    void cat(const char *name);

    // display the first N bytes of the file
    void head(const char *name, unsigned int n);

    // delete a data file
    void rm(const char *name);

    // display stats about file or directory
    void stat(const char *name);

  private:
    BasicFileSys bfs;   // basic file system
    short curr_dir; // current directory

    int fs_sock;  // file server sock

    // Additional private variables and Helper functions - if desired

    //checking if directory or file
    bool is_directory(short block_num);

    //setting message for server to send to client
    void set_message(int code, const string& message, const string& body = "");




};

#endif
