// CPSC 3500: File System
// Anwi Gundavarapu
// 6/1/25
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount(int sock) {
  bfs.mount();
  curr_dir = 1; //by default current directory is home directory, in disk block #1
  fs_sock = sock; //use this socket to receive file system operations from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name)
{

  //making sure filename is the correct size, if not returning error code
  if (strlen(name) > MAX_FNAME_SIZE) {
    set_message(504, "File name is too long");
    return;
  }


  //checking if directory name already exists
  dirblock_t dirblock;
  bfs.read_block(curr_dir, &dirblock);
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dirblock.dir_entries[i].block_num != 0 &&
        strcmp(dirblock.dir_entries[i].name, name) == 0) {
      set_message(502, "File exists");
      return;
    }
  }

  //getting a new block to allocate for directory
  //making sure there is space for directory
  short new_block = bfs.get_free_block();
  if (new_block < 0) {
    set_message(505, "Disk is full");
    return;
  }


  //Initializng block to directopry
  dirblock_t new_dir;
  new_dir.magic = DIR_MAGIC_NUM;
  new_dir.num_entries = 0;
  memset(new_dir.dir_entries, 0, sizeof(new_dir.dir_entries));
  bfs.write_block(new_block, &new_dir);

  //adding all directory data
  //flag to make sure directory entry is successfully added
  bool added = false;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dirblock.dir_entries[i].block_num == 0) {
      strncpy(dirblock.dir_entries[i].name, name, MAX_FNAME_SIZE);
      dirblock.dir_entries[i].name[MAX_FNAME_SIZE] = '\0';
      dirblock.dir_entries[i].block_num = new_block;
      dirblock.num_entries++;
      added = true;
      break;
    }
  }

  //if not added, return error code
  //returns withput commiting changes to block
  //adding entries needs to be atomic
  if (!added) {
    bfs.reclaim_block(new_block);
    set_message(506, "Directory is full");
    return;
  }

  //saving return message -- commiting changes if succesfull
  bfs.write_block(curr_dir, &dirblock);
  set_message(200, "OK");
}

// switch to a directory
void FileSys::cd(const char *name)
{

  dirblock_t dirblock;
  bfs.read_block(curr_dir, &dirblock);

  //seraching for directory
  for (int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    if (dirblock.dir_entries[i].block_num != 0 &&
      strcmp(dirblock.dir_entries[i].name, name) == 0)
    {
      short block_num = dirblock.dir_entries[i].block_num;

      //checking if directory
      //changin directory if succesfully found
      //error if not a directory
      if (is_directory(block_num))
      {
        curr_dir = block_num;
        set_message(200, "OK");
      }
      else
      {
        set_message(500, "File is not a directory");
      }
      return;
    }
  }

  //error if it doesnt exist
  set_message(503, "File does not exist");
}

// switch to home directory
void FileSys::home() {

  //setting current directory to home block
  curr_dir = 1;
  set_message(200, "OK");
}

// remove a directory
void FileSys::rmdir(const char *name)
{
  dirblock_t curr;
  bfs.read_block(curr_dir, &curr);

  //searching for directory
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (curr.dir_entries[i].block_num != 0 &&
        strcmp(curr.dir_entries[i].name, name) == 0) {
      short block_num = curr.dir_entries[i].block_num;


      //output error if not directory
      if (!is_directory(block_num)) {
        set_message(500, "File is not a directory");
        return;
      }

      //making sure directory is empty
      dirblock_t target;
      bfs.read_block(block_num, &target);
      if (target.num_entries > 0) {
        set_message(507, "Directory is not empty");
        return;
      }

      //else erasing directory data
      curr.dir_entries[i].block_num = 0;
      curr.dir_entries[i].name[0] = '\0';
      curr.num_entries--;

      bfs.reclaim_block(block_num);
      bfs.write_block(curr_dir, &curr);
      set_message(200, "OK");
      return;
    }
  }

  //error if not found
  set_message(503, "File does not exist");
}

// list the contents of current directory
void FileSys::ls()
{
  //result string that concatenates output
  string result = "";
  dirblock_t dirblock;

  bfs.read_block(curr_dir, &dirblock);

  //seraching for directory
  for (int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    if (dirblock.dir_entries[i].block_num!= 0)
    {
      char* name = dirblock.dir_entries[i].name;
      short block_num = dirblock.dir_entries[i].block_num;


      if (is_directory(block_num))
      {
        result += string(name) +"/\n";
      }
      else
      {
        result += string(name) + "\n";
      }
    }
  }

  //empty if result is emptu
  if (result.empty())
  {
    result = "empty folder\n";
  }

  //set message for server
  set_message(200, "OK", result);
}

// create an empty data file
void FileSys::create(const char *name)
{
  //making sure file name is correect size
  if (strlen(name) > MAX_FNAME_SIZE) {
    set_message(504, "File name is too long");
    return;
  }

  dirblock_t curr;
  bfs.read_block(curr_dir, &curr);

  //checking if file exists already
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (curr.dir_entries[i].block_num != 0 &&
        strcmp(curr.dir_entries[i].name, name) == 0) {
      set_message(502, "File exists");
      return;
        }
  }

  //checking if there is space for file
  //getting an inode block for file
  short inode_block = bfs.get_free_block();
  if (inode_block < 0) {
    set_message(505, "Disk is full");
    return;
  }

  //initializing inode for file
  inode_t inode{};
  inode.magic = INODE_MAGIC_NUM;
  inode.size = 0;
  bfs.write_block(inode_block, &inode);

  //adding to diretory
  bool added = false;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (curr.dir_entries[i].block_num == 0) {
      strncpy(curr.dir_entries[i].name, name, MAX_FNAME_SIZE);
      curr.dir_entries[i].name[MAX_FNAME_SIZE] = '\0';
      curr.dir_entries[i].block_num = inode_block;
      curr.num_entries++;
      added = true;
      break;
    }
  }

  //if not successful rollback
  if (!added) {
    bfs.reclaim_block(inode_block);
    set_message(506, "Directory is full");
    return;
  }

  //commit if successful
  bfs.write_block(curr_dir, &curr);
  set_message(200, "OK");
}


// append data to a data file
void FileSys::append(const char *name, const char *data)
{
  dirblock_t dir;
  bfs.read_block(curr_dir, &dir);

  //seraching for file
  short block_num = -1;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dir.dir_entries[i].block_num != 0 &&
        strcmp(dir.dir_entries[i].name, name) == 0) {
      block_num = dir.dir_entries[i].block_num;
      break;
        }
  }

  //error if file does not exist
  if (block_num == -1) {
    set_message(503, "File does not exist");
    return;
  }

  //error if file is a directory
  if (is_directory(block_num)) {
    set_message(501, "File is a directory");
    return;
  }

  //getting inode to write to file
  inode_t inode;
  bfs.read_block(block_num, &inode);

  //makign sure append is appropriate size
  int data_len = strlen(data);
  if (inode.size + data_len > MAX_FILE_SIZE) {
    set_message(508, "Append exceeds maximum file size");
    return;
  }


  int bytes_left = data_len;
  int offset = inode.size;
  int block_index = offset / BLOCK_SIZE;
  int block_offset = offset % BLOCK_SIZE;

  //making sure there is enough space on disk to write to file
  while (bytes_left > 0) {
    if (inode.blocks[block_index] == 0) {
      short new_block = bfs.get_free_block();
      if (new_block < 0) {
        set_message(505, "Disk is full");
        return;
      }
      inode.blocks[block_index] = new_block;
    }

    //datablock to write to
    datablock_t db;
    bfs.read_block(inode.blocks[block_index], &db);

    //making sure write is possible
    //copying data accordingly
    int to_copy = min(bytes_left, BLOCK_SIZE - block_offset);
    memcpy(db.data + block_offset, data + (data_len - bytes_left), to_copy);


    //write modified block back to disk
    bfs.write_block(inode.blocks[block_index], &db);

    //bookeeping updates
    //incermenting inode block for next iteration
    inode.size += to_copy;
    bytes_left -= to_copy;
    block_index++;
    block_offset = 0;
  }

  //commit write succesfully
  bfs.write_block(block_num, &inode);
  set_message(200, "OK");
}


// display the contents of a data file
void FileSys::cat(const char *name)
{
  dirblock_t dir;
  bfs.read_block(curr_dir, &dir);

  //search for file
  short inode_block = -1;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dir.dir_entries[i].block_num != 0 &&
        strcmp(dir.dir_entries[i].name, name) == 0) {
      inode_block = dir.dir_entries[i].block_num;
      break;
        }
  }

  //error if not found
  if (inode_block == -1) {
    set_message(503, "File does not exist");
    return;
  }


  //error if directory
  if (is_directory(inode_block)) {
    set_message(501, "File is a directory");
    return;
  }

  inode_t inode;
  bfs.read_block(inode_block, &inode);

  //string to store contents of file
  string content = "";
  int total_bytes = inode.size;
  int bytes_read = 0;

  //reading file to store
  //looping to read byte-by-byte
  for (int i = 0; i < MAX_DATA_BLOCKS && bytes_read < total_bytes; i++) {
    if (inode.blocks[i] == 0)
      continue;

    datablock_t db;
    bfs.read_block(inode.blocks[i], &db);

    //appending to file content according
    int to_read = min(BLOCK_SIZE, total_bytes - bytes_read);
    content.append(db.data, to_read);
    bytes_read += to_read;
  }

  //sending back content succesfully
  set_message(200, "OK", content);
}


// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
  dirblock_t dir;
  bfs.read_block(curr_dir, &dir);

  //seraching for file inode
  short block_num = -1;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dir.dir_entries[i].block_num != 0 &&
        strcmp(dir.dir_entries[i].name, name) == 0) {
       block_num = dir.dir_entries[i].block_num;
      break;
        }
  }

  //error if file not found
  if (block_num == -1) {
    set_message(503, "File does not exist");
    return;
  }

  //error if file is a directory
  if (is_directory(block_num)) {
    set_message(501, "File is a directory");
    return;
  }

  inode_t inode;
  bfs.read_block(block_num, &inode);

  //storing first N bytes' content
  string content = "";
  int total_bytes = min((unsigned int)inode.size, n);
  int bytes_read = 0;

  //looping to append till total_bytes
  //appending conent appropriately
  for (int i = 0; i < MAX_DATA_BLOCKS && bytes_read < total_bytes; i++) {
    if (inode.blocks[i] == 0)
      continue;

    datablock_t db;
    bfs.read_block(inode.blocks[i], &db);

    int to_read = min(BLOCK_SIZE, total_bytes - bytes_read);
    content.append(db.data, to_read);
    bytes_read += to_read;
  }

  //succesfully set output
  set_message(200, "OK", content);
}


// delete a data file
void FileSys::rm(const char *name)
{
  dirblock_t dir;
  bfs.read_block(curr_dir, &dir);

  //searching for file
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dir.dir_entries[i].block_num != 0 &&
        strcmp(dir.dir_entries[i].name, name) == 0) {

      short block_num = dir.dir_entries[i].block_num;

      //appropriate message if file is a directory
      if (is_directory(block_num)) {
        set_message(501, "File is a directory");
        return;
      }

      inode_t inode;
      bfs.read_block(block_num, &inode);

      //reclaiming allocated inode blocks for this file
      for (int j = 0; j < MAX_DATA_BLOCKS; j++) {
        if (inode.blocks[j] != 0)
          bfs.reclaim_block(inode.blocks[j]);
      }

      bfs.reclaim_block(block_num);

      //removing/nulling out file entry from direcryot
      dir.dir_entries[i].block_num = 0;
      dir.dir_entries[i].name[0] = '\0';
      dir.num_entries--;

      //commit delete
      bfs.write_block(curr_dir, &dir);
      set_message(200, "OK");
      return;
    }
  }

  //error if file not found
  set_message(503, "File does not exist");
}


// display stats about file or directory
void FileSys::stat(const char *name)
{
  dirblock_t dir;
  bfs.read_block(curr_dir, &dir);

  //search for file/directory
  for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
    if (dir.dir_entries[i].block_num != 0 &&
        strcmp(dir.dir_entries[i].name, name) == 0) {

      short block_num = dir.dir_entries[i].block_num;

      if (is_directory(block_num)) {
        //storing directroy stats and returning message
        string stats = "Directory name: " + string(name) + "\n";
        stats += "Directory block: " + to_string(block_num) + "\n";
        set_message(200, "OK", stats);
        return;
      }

      //must be a file
      inode_t inode;
      bfs.read_block(block_num, &inode);


      //if file, count how many data blocks are used,
      //then output appropriately
      if (!is_directory(block_num)) {
        int block_count = 0;
        short first_data_block = 0;

        for (int j = 0; j < MAX_DATA_BLOCKS; j++) {
          if (inode.blocks[j] != 0) {
            if (first_data_block == 0)
              first_data_block = inode.blocks[j];
            block_count++;
          }
        }

        if (inode.size == 0)
          first_data_block = 0;

        //store stats and return with display message
        string stats = "Inode block: " + to_string(block_num) + "\n";
        stats += "Bytes in file: " + to_string(inode.size) + "\n";
        stats += "Number of blocks: " + to_string(block_count) + "\n";
        stats += "First block: " + to_string(first_data_block) + "\n";

        set_message(200, "OK", stats);
        return;
      }

      set_message(500, "Unknown file type");
      return;
        }
  }

  set_message(503, "File does not exist");
}


// HELPER FUNCTIONS (optional)

//checls if directroy by checkinf the magic number
bool FileSys::is_directory(short block_num) {
  dirblock_t block;
  bfs.read_block(block_num, &block);
  return block.magic == DIR_MAGIC_NUM;
}

void FileSys::set_message(int code, const string& message, const string& body)
{
  //stores output to response so server can recieve
  response = to_string(code) + " " + message + "\r\n";
  response += "Length:" + to_string(body.size()) + "\r\n";
  response += "\r\n";
  response += body;
}


