#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>

#define NUM_SERVERS 1
#define BASE_PORT 8890
#define MAX_MESSAGE_SIZE 20000
#define MAX_STAT_RESPONSE_SIZE 512

// Server operations
int create_server_socket(int port);
void* server_thread(void* arg);
void* handle_client(void* arg);


// File sytem operations
#define block_size 1024

typedef struct superblock {
	char datablocks[block_size*100];		//total number of data blocks
	char data_bitmap[105];      			//array of data block numbers that are available
	char inode_bitmap[105];   				//array of inode numbers that are available
} superblock;

typedef struct inode {
	int datablocks[16];            //data block number that the inode points to
	int number;
	int blocks;                    //==number of blocks the particular inode points to
	int size;                      //==size of file/directory
} inode;

typedef struct filetype {
	int valid;
	char test[10];
	char path[100];
	char name[100];           //name
	inode *inum;              //inode number
	struct filetype ** children;
	int num_children;
	int num_links;
	struct filetype * parent;
	char type[20];                  //==file extension
	mode_t permissions;		        // Permissions
	uid_t user_id;		            // userid
	gid_t group_id;		            // groupid
	time_t a_time;                  // Access time
	time_t m_time;                  // Modified time
	time_t c_time;                  // Status change time
	time_t b_time;                  // Creation time
	off_t size;                     // Size of the node

	int datablocks[16];
	int number;
	int blocks;

} filetype;

filetype * root;
filetype file_array[200];
superblock spblock;


// File system operations
void initialize_superblock();
void tree_to_array(filetype * queue, int * front, int * rear, int * index);
int save_contents();
void initialize_root_directory();
filetype * filetype_from_path(char * path);
int find_free_inode();
int find_free_db();
void add_child(filetype * parent, filetype * child);
static int fsgetattr(const char *path, struct stat *statit);
static int fsmkdir(const char *path, mode_t mode);
int fsreaddir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi );
int fscreate(const char * path, mode_t mode, struct fuse_file_info *fi);
int fsopen(const char *path, struct fuse_file_info *fi);
int fsread(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fswrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fsrelease(const char *path, struct fuse_file_info *fi);
static int filler_function(void *buffer, const char *name, const struct stat *stbuf, off_t off);


// Error handling
void error(const char* message);


// Get attributes of a file or directory
int get_fileattr(const char *path, struct stat *statbuf) {
    return fsgetattr(path, statbuf);
}

// Create a directory
int create_dir(const char *path, mode_t mode) {
    return fsmkdir(path, mode);
}

// Create a file
int create_file(const char *path, mode_t mode) {
    struct stat stbuf;

    // Call getattr to check if file already exists
    if (fsgetattr(path, &stbuf) != -1) {
        return -1;
    }

    // Call fscreate to create the file
    int create_result = fscreate(path, mode, NULL);
    if (create_result == -1) {
        // Error creating file, return error
        return -1;
    }

    return 0; // File created successfully
}


void initialize_superblock(){

	memset(spblock.data_bitmap, '0', 100*sizeof(char));
	memset(spblock.inode_bitmap, '0', 100*sizeof(char));
}


void tree_to_array(filetype * queue, int * front, int * rear, int * index){

	if(rear < front)
		return;
	if(*index > 30)
		return;


	filetype curr_node = queue[*front];
	*front += 1;
	file_array[*index] = curr_node;
	*index += 1;

	if(*index < 6){

		if(curr_node.valid){
			int n = 0;
			int i;
			for(i = 0; i < curr_node.num_children; i++){
				if(*rear < *front)
					*rear = *front;
				queue[*rear] = *(curr_node.children[i]);
				*rear += 1;
			}
			while(i<5){
				filetype waste_node;
				waste_node.valid = 0;
				queue[*rear] = waste_node;
				*rear += 1;
				i++;
			}
		}
		else{
			int i = 0;
			while(i<5){
				filetype waste_node;
				waste_node.valid = 0;
				queue[*rear] = waste_node;
				*rear += 1;
				i++;
			}
		}
	}

	tree_to_array(queue, front, rear, index);

}


int save_contents(){
	printf("Saving file\n");
	filetype * queue = malloc(sizeof(filetype)*60);
	int front = 0;
	int rear = 0;
	queue[0] = *root;
	int index = 0;
	tree_to_array(queue, &front, &rear, &index);

	for(int i = 0; i < 31; i++){
		printf("%d", file_array[i].valid);
	}

	FILE * fd = fopen("file_structure.bin", "wb");

	FILE * fd1 = fopen("super.bin", "wb");

	fwrite(file_array, sizeof(filetype)*31, 1, fd);
	fwrite(&spblock,sizeof(superblock),1,fd1);

	fclose(fd);
	fclose(fd1);

	printf("\n");
}

void initialize_root_directory() {

	spblock.inode_bitmap[1]=1; //marking it with 0
	root = (filetype *) malloc (sizeof(filetype));

	strcpy(root->path, "/");
	strcpy(root->name, "/");

	root -> children = NULL;
	root -> num_children = 0;
	root -> parent = NULL;
	root -> num_links = 2;
	root -> valid = 1;
	strcpy(root->test, "test");
	strcpy(root -> type, "directory");

	root->c_time = time(NULL);
	root->a_time = time(NULL);
	root->m_time = time(NULL);
	root->b_time = time(NULL);

	root -> permissions = S_IFDIR | 0777;

	root -> size = 0;
	root->group_id = getgid();
	root->user_id = getuid();

	root -> number = 2;
	root -> blocks = 0;

	save_contents();
}

filetype * filetype_from_path(char * path){
	char curr_folder[100];
	char * path_name = malloc(strlen(path) + 2);

	strcpy(path_name, path);

	filetype * curr_node = root;

	fflush(stdin);

	if(strcmp(path_name, "/") == 0)
		return curr_node;

	if(path_name[0] != '/'){
		printf("INCORRECT PATH\n");
		exit(1);
	}
	else{
		path_name++;
	}

	if(path_name[strlen(path_name)-1] == '/'){
		path_name[strlen(path_name)-1] = '\0';
	}

	char * index;
	int flag = 0;

	while(strlen(path_name) != 0){
		index = strchr(path_name, '/');
		
		if(index != NULL){
			strncpy(curr_folder, path_name, index - path_name);
			curr_folder[index-path_name] = '\0';
			
			flag = 0;
			for(int i = 0; i < curr_node -> num_children; i++){
				if(strcmp((curr_node -> children)[i] -> name, curr_folder) == 0){
					curr_node = (curr_node -> children)[i];
					flag = 1;
					break;
				}
			}
			if(flag == 0)
				return NULL;
		}
		else{
			strcpy(curr_folder, path_name);
			flag = 0;
			for(int i = 0; i < curr_node -> num_children; i++){
				if(strcmp((curr_node -> children)[i] -> name, curr_folder) == 0){
					curr_node = (curr_node -> children)[i];
					return curr_node;
				}
			}
			return NULL;
		}
		path_name = index+1;
	}

}

int find_free_inode(){
	for (int i = 2; i < 100; i++){
		if(spblock.inode_bitmap[i] == '0'){
			spblock.inode_bitmap[i] = '1';
		}
		return i;
	}
}

int find_free_db(){
	for (int i = 1; i < 100; i++){
		if(spblock.inode_bitmap[i] == '0'){
			spblock.inode_bitmap[i] = '1';
		}
		return i;
	}
}

void add_child(filetype * parent, filetype * child){
	(parent -> num_children)++;

	parent -> children = realloc(parent -> children, (parent -> num_children)*sizeof(filetype *));

	(parent -> children)[parent -> num_children - 1] = child;
}


static int fsgetattr(const char *path, struct stat *statit) {
	char *pathname;
	pathname=(char *)malloc(strlen(path) + 2);

	strcpy(pathname, path);

	printf("GETATTR %s\n", pathname);

	filetype * file_node = filetype_from_path(pathname);
	if(file_node == NULL)
		return -ENOENT;

	statit->st_uid = file_node -> user_id; // The owner of the file/directory is the user who mounted the filesystem
	statit->st_gid = file_node -> group_id; // The group of the file/directory is the same as the group of the user who mounted the filesystem
	statit->st_atime = file_node -> a_time; // The last "a"ccess of the file/directory is right now
	statit->st_mtime = file_node -> m_time; // The last "m"odification of the file/directory is right now
	statit->st_ctime = file_node -> c_time;
	statit->st_mode = file_node -> permissions;
	statit->st_nlink = file_node -> num_links + file_node -> num_children;
	statit->st_size = file_node -> size;
	statit->st_blocks = file_node -> blocks;

	return 0;
}


static int fsmkdir(const char *path, mode_t mode) {
	printf("MKDIR\n");

	int index = find_free_inode();

	filetype * new_folder = malloc(sizeof(filetype));

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	char * rindex = strrchr(pathname, '/');

	strcpy(new_folder -> name, rindex+1);
	strcpy(new_folder -> path, pathname);

	*rindex = '\0';

	if(strlen(pathname) == 0)
	strcpy(pathname, "/");

	new_folder -> children = NULL;
	new_folder -> num_children = 0;
	new_folder -> parent = filetype_from_path(pathname);
	new_folder -> num_links = 2;
	new_folder -> valid = 1;
	strcpy(new_folder -> test, "test");

	if(new_folder -> parent == NULL)
		return -ENOENT;


	add_child(new_folder->parent, new_folder);

	strcpy(new_folder -> type, "directory");

	new_folder->c_time = time(NULL);
	new_folder->a_time = time(NULL);
	new_folder->m_time = time(NULL);
	new_folder->b_time = time(NULL);

	new_folder -> permissions = S_IFDIR | 0777;

	new_folder -> size = 0;
	new_folder->group_id = getgid();
	new_folder->user_id = getuid();


	new_folder -> number = index;
	new_folder -> blocks = 0;


	save_contents();

	return 0;

}


int fsreaddir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ){
	fi = malloc(sizeof(struct fuse_file_info));

	if (fi == NULL){
		perror("Unable to allocate memory for fuse_file_info");
		return -1;
	}

	memset(fi, 0, sizeof(struct fuse_file_info));

    initialize_superblock();
    printf("READDIR\n");

	filler(buffer, ".", NULL, 0 );
	filler(buffer, "..", NULL, 0 );

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	filetype * dir_node = filetype_from_path(pathname);

	if(dir_node == NULL){
		return -ENOENT;
	}
	else{
		dir_node->a_time=time(NULL);
		for(int i = 0; i < dir_node->num_children; i++){
			printf(":%s:\n", dir_node->children[i]->name);
			filler( buffer, dir_node->children[i]->name, NULL, 0 );
		}
	}

	return 0;
}


int fscreate(const char * path, mode_t mode, struct fuse_file_info *fi) {

	printf("CREATEFILE\n");

	int index = find_free_inode();

	filetype * new_file = malloc(sizeof(filetype));

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	char * rindex = strrchr(pathname, '/');

	strcpy(new_file -> name, rindex+1);
	strcpy(new_file -> path, pathname);

	*rindex = '\0';

	if(strlen(pathname) == 0)
		strcpy(pathname, "/");

	new_file -> children = NULL;
	new_file -> num_children = 0;
	new_file -> parent = filetype_from_path(pathname);
	new_file -> num_links = 0;
	new_file -> valid = 1;

	if(new_file -> parent == NULL)
	return -ENOENT;

	add_child(new_file->parent, new_file);

	strcpy(new_file -> type, "file");

	new_file->c_time = time(NULL);
	new_file->a_time = time(NULL);
	new_file->m_time = time(NULL);
	new_file->b_time = time(NULL);

	new_file -> permissions = S_IFREG | 0777;

	new_file -> size = 0;
	new_file->group_id = getgid();
	new_file->user_id = getuid();


	new_file -> number = index;

	for(int i = 0; i < 16; i++){
		(new_file -> datablocks)[i] = find_free_db();
	}

	new_file -> blocks = 0;

	save_contents();

	return 0;
}


int fsopen(const char *path, struct fuse_file_info *fi) {
	printf("OPEN\n");

	char * pathname = malloc(strlen(path)+1);
    if (pathname == NULL) {
        perror("malloc");
        return -1;
    }
	strcpy(pathname, path);

	filetype * file = filetype_from_path(pathname);

    free(pathname);
	return 0;
}


int fsread(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
    // Call initialize_superblock before starting the server
    initialize_superblock();

	printf("READ\n");

	char * pathname = malloc(strlen(path)+1);
	strcpy(pathname, path);

	filetype * file = filetype_from_path(pathname);
	if(file == NULL)
		return -ENOENT;

	else{
		char * str = malloc(sizeof(char)*1024*(file -> blocks));

		printf(":%ld:\n", file->size);
		strcpy(str, "");
		int i;
		for(i = 0; i < (file -> blocks) - 1; i++){
			strncat(str, &spblock.datablocks[block_size*(file -> datablocks[i])], 1024);
			printf("--> %s", str);
		}
		strncat(str, &spblock.datablocks[block_size*(file -> datablocks[i])], (file -> size)%1024);
		printf("--> %s", str);
		strcpy(buf, str);
	}
	return file->size;
}


int fswrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	printf("WRITING\n");

	char * pathname = malloc(strlen(path)+1);
	strcpy(pathname, path);

	filetype * file = filetype_from_path(pathname);
	if(file == NULL)
		return -ENOENT;

	int indexno = (file->blocks)-1;

	if(file -> size == 0){
		strcpy(&spblock.datablocks[block_size*((file -> datablocks)[0])], buf);
		file -> size = strlen(buf);
		(file -> blocks)++;
	}
	else{
		int currblk = (file->blocks)-1;
		int len1 = 1024 - (file -> size % 1024);
		if(len1 >= strlen(buf)){
			strcat(&spblock.datablocks[block_size*((file -> datablocks)[currblk])], buf);
			file -> size += strlen(buf);
			printf("---> %s\n", &spblock.datablocks[block_size*((file -> datablocks)[currblk])]);
		}
		else{
			char * cpystr = malloc(1024*sizeof(char));
            if (cpystr == NULL) {
                // Handle memory allocation failure
                return -ENOMEM; // or another appropriate error code
            }
			strncpy(cpystr, buf, len1-1);
			strcat(&spblock.datablocks[block_size*((file -> datablocks)[currblk])], cpystr);
			strcpy(cpystr, buf);
			strcpy(&spblock.datablocks[block_size*((file -> datablocks)[currblk+1])], (cpystr+len1-1));
			file -> size += strlen(buf);
			printf("---> %s\n", &spblock.datablocks[block_size*((file -> datablocks)[currblk])]);
			(file -> blocks)++;

            // Deallocate memory for cpystr
            free(cpystr);
		}

	}
	save_contents();

	return strlen(buf);
}


static int fsrelease(const char *path, struct fuse_file_info *fi) {
    (void) path; // Unused parameter
    close(fi->fh);

    return 0; 
}


// Function to send error response to the client
void send_error_response_to_client(const char* error_message, int client_sock) {
    if (write(client_sock, error_message, strlen(error_message)) < 0) {
        perror("Error writing to client socket");
    }
}


// Function to send success response to the client
void send_success_response_to_client(const char* success_message, int client_sock) {
    if (write(client_sock, success_message, strlen(success_message)) < 0) {
        perror("Error writing to client socket");
    }
}


static struct fuse_operations fsoperations =
{
    .getattr=fsgetattr,     //test with stat <filename>
	.mkdir=fsmkdir,         //test with mkdir <dirname>
	.readdir=fsreaddir,     //test with ls
	.open=fsopen,           //test with cat <filename>
	.read=fsread,           //test with cat <filename>
	.create=fscreate,       //test with touch <filename>
    .write=fswrite,         //test with echo "hello" >> <filename>
	.release=fsrelease,     //
};


void* server_thread(void* arg) {
    // Call initialize_superblock before starting the server
    initialize_superblock();

    int port = *((int*) arg);
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("Could not create socket");
        pthread_exit(NULL);
    }
    
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("10.10.59.53");
	server.sin_port = htons(8890);
    
    // Bind
    if( bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        close(socket_desc);
        pthread_exit(NULL);
    }
    
    // Listen
    if (listen(socket_desc, 3) < 0) {
        perror("listen failed");
        close(socket_desc);
        pthread_exit(NULL);
    }
    
    // Accept and handle incoming connections
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    while (1) {
        // Accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }
        printf("Connection accepted on port %d\n", port);
        
        // Create a new thread to handle client request
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void*)(intptr_t)client_sock) != 0) {
            perror("Failed to create client thread");
            close(client_sock);
            continue;
        }
        
        // Detach the client thread
        pthread_detach(client_thread);
    }
    
    // Close server socket before exiting
    close(socket_desc);
    pthread_exit(NULL);
}


static int filler_function(void *buffer, const char *name, const struct stat *stbuf, off_t off) {
    return fuse_add_direntry(buffer, MAX_MESSAGE_SIZE, name, stbuf, off);
}


// Function to handle client request
void* handle_client(void* arg) {
    int client_sock = (intptr_t)arg;
    int read_size;
    char client_message[MAX_MESSAGE_SIZE];
    
    // Receive a message from client
    while ((read_size = recv(client_sock , client_message , sizeof(client_message) , 0)) > 0 ) {
        // Parse the received message (assuming format: "OPERATION filename")
        char operation[MAX_MESSAGE_SIZE];
        char filename[MAX_MESSAGE_SIZE];

        if (sscanf(client_message, "%s %s", operation, filename) != 2) {
            // Invalid request format, send error response
            const char *error_message = "Invalid request format";
            if (write(client_sock, error_message, strlen(error_message)) < 0) {
                perror("Error writing to client socket");
            }
            close(client_sock);
            return NULL;
        }

				// Handle open file request
		if (strcmp(operation, "ACCESS") == 0) {
			struct stat file_stat;
			int result = fsgetattr(filename, &file_stat); 
			struct fuse_file_info fi;
        	memset(&fi, 0, sizeof(fi));

			if (result == 0) {
				// File found within FUSE filesystem
				int fd = fsopen(filename, &fi); 
				if (fd >= 0) {
					// File opened successfully, send contents
					char *buffer = (char *)malloc(MAX_MESSAGE_SIZE * sizeof(char));
					ssize_t bytes_read;

					while ((bytes_read = fsread(filename, buffer, MAX_MESSAGE_SIZE, 0, &fi)) > 0) {
						if (write(client_sock, buffer, bytes_read) < 0) {
							perror("Error writing to client socket");
							break;
						}
					}
					free(buffer);
					if (bytes_read < 0) {
						const char *error_message = "Error reading file";
						write(client_sock, error_message, strlen(error_message)); 
					}
				} else {
					const char *error_message = "Error opening file"; 
					write(client_sock, error_message, strlen(error_message));
				}
			} else {
				const char *error_message = "File not found";
				write(client_sock, error_message, strlen(error_message));
			}
			close(client_sock);
		}
		else if (strcmp(operation, "CREATEFILE") == 0) {
			// Check if the file already exists
			struct stat file_stat;
			int result = fsgetattr(filename, &file_stat);
			if (result == 0) {
				// File already exists, send error response to client
				const char *error_message = "File already exists";
				write(client_sock, error_message, strlen(error_message));
			} else {
				// File doesn't exist, attempt to create the file
				struct fuse_file_info fi;
				memset(&fi, 0, sizeof(fi));
				int result = fscreate(filename, 0666, &fi); // Hardcoded mode 0666 for read/write permissions
				if (result == 0) {
					// File created successfully, send success response to client
					const char *success_message = "File created successfully";
					write(client_sock, success_message, strlen(success_message));
				} else {
					// Error creating file, send error response to client
					const char *error_message = "Error creating file";
					write(client_sock, error_message, strlen(error_message));
				}
			}
		}
		else if (strcmp(operation, "WRITE") == 0) {
			// Extract the text to write
			char text[MAX_MESSAGE_SIZE];
			char filepath[MAX_MESSAGE_SIZE];
			if (sscanf(client_message, "%*s \"%[^\"]\" %s", text, filepath) != 2) {
				// Invalid request format, send error response
				const char *error_message = "Invalid WRITE request format";
				write(client_sock, error_message, strlen(error_message));
				close(client_sock);
				return NULL;
			}
			
			// Call fswrite to write to the file
			int result = fswrite(filepath, text, strlen(text), 0, NULL);
			if (result >= 0) {
				// Write successful, send success response to client
				const char *success_message = "Write operation successful";
				write(client_sock, success_message, strlen(success_message));
			} else {
				// Error writing to file, send error response to client
				const char *error_message = "Error writing to file";
				write(client_sock, error_message, strlen(error_message));
			}
			close(client_sock);
		}
		else if (strcmp(operation, "FILESTAT") == 0) {
			// Extract the filepath from the client message
			char filepath[MAX_MESSAGE_SIZE];
			if (sscanf(client_message, "%*s %s", filepath) != 1) {
				// Invalid request format, send error response
				const char *error_message = "Invalid STAT request format";
				write(client_sock, error_message, strlen(error_message));
				close(client_sock);
				return NULL;
			}
			
			// Call fsgetattr to get file attributes
			struct stat file_stat;
			int result = fsgetattr(filepath, &file_stat);
			if (result == 0) {
				// File attributes retrieved successfully, send response to client
				// Convert file_stat to a string representation
				char stat_str[MAX_STAT_RESPONSE_SIZE];
				snprintf(stat_str, MAX_STAT_RESPONSE_SIZE, "Name: %s\nFile size: %lld, Permissions: %o", filepath, (long long)file_stat.st_size, file_stat.st_mode);
				write(client_sock, stat_str, strlen(stat_str));
			} else {
				// Error getting file attributes, send error response to client
				const char *error_message = "Error getting file attributes";
				write(client_sock, error_message, strlen(error_message));
			}
			close(client_sock);
		}
		else if (strcmp(operation, "MKDIR") == 0) {
			// Check if the directory already exists
			struct stat dir_stat;
			int result = fsgetattr(filename, &dir_stat);
			if (result == 0) {
				// Directory already exists, send error response to client
				const char *error_message = "Directory already exists";
				write(client_sock, error_message, strlen(error_message));
			} else {
				// Directory doesn't exist, attempt to create the directory
				result = fsmkdir(filename, 0777); // Hardcoded mode 0777 for directory permissions
				if (result == 0) {
					// Directory created successfully, send success response to client
					const char *success_message = "Directory created successfully";
					write(client_sock, success_message, strlen(success_message));
				} else {
					// Error creating directory, send error response to client
					const char *error_message = "Error creating directory";
					write(client_sock, error_message, strlen(error_message));
				}
			}
			close(client_sock);
		}
		else if (strcmp(operation, "LISTDIR") == 0) {
			// Check if the directory exists
			struct stat dir_stat;
			int result = fsgetattr(filename, &dir_stat);
			if (result != 0) {
				// Directory does not exist, send error response to client
				const char *error_message = "Directory not found";
				write(client_sock, error_message, strlen(error_message));
				return NULL;
			}

			// Directory exists, attempt to read its contents
			struct fuse_file_info fi;
			memset(&fi, 0, sizeof(fi));
			void *buffer = malloc(MAX_MESSAGE_SIZE); // Allocate buffer for directory listing
			if (buffer == NULL) {
				perror("Error allocating memory for directory listing buffer");
				const char *error_message = "Error listing directory";
				write(client_sock, error_message, strlen(error_message));
				return NULL;
			}

			int bytes_read = fsreaddir(filename, buffer, filler_function, 0, &fi);
			if (bytes_read >= 0) {
				// Directory contents read successfully, send response to client
				if (write(client_sock, buffer, bytes_read) < 0) {
					perror("Error writing directory contents to client socket");
				}
			} else {
				// Error reading directory contents, send error response to client
				const char *error_message = "Error listing directory";
				write(client_sock, error_message, strlen(error_message));
			}

			free(buffer); // Free the allocated buffer
			return 0;
		}
		else {
			// Invalid operation, send error response
			const char *error_message = "Invalid operation";
			if (write(client_sock, error_message, strlen(error_message)) < 0) {
				perror("Error writing to client socket");
			}
		} 

    }
    
    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if(read_size < 0) {
        perror("recv failed");
    }
    
    close(client_sock); // Close the client socket
    
    return NULL;
}


int initialize_filesystem() {
    FILE *fd = fopen("file_structure.bin", "rb");
    if (fd) {
        printf("LOADING\n");
        fread(&file_array, sizeof(filetype)*31, 1, fd);

        int child_startindex = 1;
        file_array[0].parent = NULL;

        for (int i = 0; i < 6; i++) {
            file_array[i].num_children = 0;
            file_array[i].children = NULL;
            for (int j = child_startindex; j < child_startindex + 5; j++) {
                if (file_array[j].valid) {
                    add_child(&file_array[i], &file_array[j]);
                }
            }
            child_startindex += 5;
        }

        root = &file_array[0];

        FILE *fd1 = fopen("super.bin", "rb");
        fread(&spblock, sizeof(superblock), 1, fd1);
    } else {
        initialize_superblock();
        initialize_root_directory();
    }
}

int main(int argc, char *argv[]) {
    // Initialize the file system
    initialize_filesystem();

    // Check if DFS tree initialization failed
    if (root == NULL) {
        fprintf(stderr, "Failed to initialize DFS tree\n");
        return 1;
    }

    // Create and start server threads
    pthread_t server_threads[NUM_SERVERS];
    int ports[NUM_SERVERS];

    for (int i = 0; i < NUM_SERVERS; i++) {
        ports[i] = BASE_PORT + i;
        if (pthread_create(&server_threads[i], NULL, server_thread, (void*)&ports[i]) != 0) {
            perror("Failed to create server thread");
            return 1;
        }
        printf("Server %d started on port %d\n", i+1, ports[i]);
        printf("Mounting FUSE filesystem on server(s)...\n");
        // Start the FUSE filesystem after the server threads are created
        if (fuse_main(argc, argv, &fsoperations, NULL) != 0) {
            fprintf(stderr, "Fuse filesystem exited with error\n");
            return 1; // or any other appropriate error code
        } 
    }

    // Wait for server threads to finish
    for (int i = 0; i < NUM_SERVERS; i++) {
        pthread_join(server_threads[i], NULL);
    }

    return 0;
}
