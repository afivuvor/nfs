
## Simulated Network File System
This is a simulated network file system implemented using FUSE (Filesystem in Userspace) and socket programming in C. This simulated network file system supports basic file operations such as:  
- creating files 
- reading from files
- writing into files

## Youtube tutorial 
This is a link to  [youtube video] demonstrating usage of this system.

## Features
- **File System Operations**: Supports various file system operations including file creation, directory creation, file reading, file writing, directory listing, and file access control.

- **Data Persistence**: File system structure and metadata are stored persistently in binary files (`file_structure.bin` and `super.bin`) to maintain data integrity across server restarts.

- **Remote Connection**: The file system is accessible to the client from a remote server over a network.


## Usage

To use the file system, follow these steps:
1. **Setting up client-server**: This simulation requires two machines; one to act as a  server and the other as a client. The server should preferably run a unix operating system like Ubuntu.
2. **Network set-up**: Two ethernet cables would be required to enable client server communicate over a local area network. The two ethernet cables should connnect each computer to the same network.
3. **Network configuration**: The internet protocol used in this simulation is the IPv4. To bind the sockets of the server and the client, utilize the IPv4 address found in the network settings of the two devices as shown below. The client socket and the server socket should be bound on the same port.

   ```
   //Server code
   #define BASE_PORT 8890
    server.sin_addr.s_addr = inet_addr("10.10.60.32");
   ```
   
   ```
   //Client code
   server.sin_addr.s_addr = inet_addr("10.10.60.32"); 
   server.sin_port = htons(8890); 
   ```
4. **Compile the server**: Compile the code using the gcc compiler. Ensure that the pkg-config helper tool and fuse package are installed on both the server machine by running: 
   
   ```
   sudo apt install libfuse-dev 
   gcc fileServer.c -g -o fileServer pkg-config fuse --cflags --libs
   ./fileServer
   ```
   
5. **Mount the file system**: Mount the file system on the server using the code below
   
   ```
   gcc fileServer.c -g -o fileServer pkg-config fuse --cflags --libs
   ./fileServer
   ```

6. **Compile and run the client**: Run the compiled executable to initialize the file system.

   ```
   gcc fileClient.c -g -o fileClient pkg-config fuse --cflags --libs
   ./fileClient
   ```

7.  **Access the File System**: Use the suggested file system commands presented to access your remote files and directories.

       ```
       Enter desired file system command: CREATEFILE /senam.txt
       Enter desired file system command: WRITE hello /senam.txt 
       Enter desired file system command: ACCESS /senam.txt
       ```
  


## File System Structure

The file system maintains a hierarchical structure consisting of directories and files. Each file or directory is represented by a `filetype` struct, which contains metadata such as name, path, permissions, timestamps, and data blocks. As consistent with real file systems, this file systems takes the structure of a tree, with the directories as nodes that have children(files). 


## Error Handling

The file system handles various error conditions gracefully and provides informative error messages to clients when operations fail.

## Dependencies

- FUSE library
- POSIX threads library
- Package config helper tool
- Libfuse dev package
- Net-tools package

## Contributors

- Sena Afi Vuvor
- Senam Glover-Tay


## License

MIT


   [youtube video]: <https://youtu.be/gy8wmt6tlMo>


## References
Primary Debugging tool: 
chat.openai.com

FUSE Implementation: 
https://libfuse.github.io/doxygen/index.html
https://github.com/Hasil-Sharma/distributed-file-system/tree/master
https://github.com/Aveek-Saha/FUSE-Filesystem/tree/master