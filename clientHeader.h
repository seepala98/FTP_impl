#define NUM_INPUT 3
#define BUFFER_SIZE 1024
#define PORT_NUM 5000

int cmdRunner(char *buffer, char *command, char *input, int server_fd, int *data_port);
void displayHelper(int server_fd);
int portClient(int server_fd, int *data_port);
int tcpHelper(int *data_port, char *IPInfo);
int storHelperClientSide(char *filename, int srv_socket);
int retrHelperClient(char *filename, int srv_socket);
int listHelperPwdClient(char *command, int srv_socket);
int cdupHelper(char *command, int srv_socket);
void noopHelperClient(int server_fd);
void deleHelperClient(char *filename, int server_fd);