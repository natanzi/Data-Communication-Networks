
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MESSAGE_SIZE 1024

struct sockaddr_in server_addr, client_addr;

int create_socket(int port_number) {
    // Creating the UDP Socket
    int descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    // Terminating the program if socket creation failed.
    if (descriptor < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Filling out the struct with null bytes.
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    // Setting Socket Properties
    server_addr.sin_family = AF_INET; // server_addr family to communicate with
    server_addr.sin_addr.s_addr = INADDR_ANY;  // accept any incoming messages
    server_addr.sin_port = htons(port_number);
    // Binding the Socket to the Port
    int returned = bind(descriptor, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
    if (returned < 0) {
        perror("socket bind failed");
        exit(EXIT_FAILURE);
    }
    // Returning the Descriptor for Further Use
    return descriptor;
}

int random_in_range(int a, int b) {
    return a + (std::rand() % (b - a + 1));
}

int main(int argc, char *argv[]) {
    // Command Line Arguments
    int n_port = atoi(argv[1]);
    // Creating the Initial Socket
    int descriptor = create_socket(n_port);
    // Declaring the Necessary Variables for Communication
    char buffer[MESSAGE_SIZE], copy_buffer[MESSAGE_SIZE];
    ssize_t bytes_read, bytes_sent;
    socklen_t len;
    // Waiting for Handshake
    len = sizeof(client_addr);
    memset(buffer, 0, MESSAGE_SIZE);
    bytes_read = recvfrom(descriptor, buffer, MESSAGE_SIZE, MSG_WAITALL,
                          reinterpret_cast<sockaddr *>(&client_addr), &len);
    if (bytes_read < 0) {  // recvfrom failed to receive any bytes properly.
        perror("receive failed");
        exit(EXIT_FAILURE);
    }
    if (strcmp(buffer, "ABC") != 0) {  // recvfrom did receive something, but it's not "ABC".
        perror("handshake failed");
        exit(EXIT_FAILURE);
    }
    // Selecting a random port and printing it out.
    int r_port = random_in_range(1024, 65536);  // Our function is non-inclusive, so the upper bound is one more than the maximum.
    std::cout << '\n' << "Random port: " << r_port << '\n' << std::endl;
    // Letting the client know what port we have selected.
    memset(buffer, 0, MESSAGE_SIZE);
    sprintf(buffer, "%d", r_port);
    bytes_sent = sendto(descriptor, buffer, MESSAGE_SIZE, MSG_CONFIRM,
                        reinterpret_cast<const sockaddr *>(&client_addr), sizeof(client_addr));
    if (bytes_sent < 0) {  // sendto failed to send any bytes properly.
        perror("send failed");
        exit(EXIT_FAILURE);
    }
    // Closing the Current Socket
    close(descriptor);
    // Creating a Stream for Saving the Data
    std::ofstream outfile("upload.txt");
    // Creating a New Socket
    descriptor = create_socket(r_port);
    // Waiting in Loop for New Chunks
    bool data_incoming = true;
    while (data_incoming) {
        len = sizeof(client_addr);
        memset(buffer, 0, MESSAGE_SIZE);
        bytes_read = recvfrom(descriptor, buffer, MESSAGE_SIZE, MSG_WAITALL,
                              reinterpret_cast<sockaddr *>(&client_addr), &len);
        if (bytes_read < 0) {  // recvfrom failed to receive any bytes properly.
            perror("receive failed");
            exit(EXIT_FAILURE);
        }
        // In our communication protocol, the initial byte specifies the number of  bytes to be read from the chunk,
        // in case there are less than 4 valid bytes. If it is zero, we have hit the end of the file.
        int valid_bytes = static_cast<int>(buffer[0]);
        if (valid_bytes > 0)
            outfile.write(buffer + 1, valid_bytes);
        // Preparing Acknowledgement Message
        memset(copy_buffer, 0, MESSAGE_SIZE);
        for (int i = 0; i < valid_bytes; i++)
            copy_buffer[i] = static_cast<char>(toupper(buffer[i + 1]));
        // Sending Acknowledgement Message
        bytes_sent = sendto(descriptor, copy_buffer, MESSAGE_SIZE, MSG_CONFIRM,
                            reinterpret_cast<const sockaddr *>(&client_addr), sizeof(client_addr));
        if (bytes_sent < 0) {  // sendto failed to send the bytes properly.
            perror("acknowledgement failed");
            exit(EXIT_FAILURE);
        }
        // Deciding whether to continue the loop.
        data_incoming = valid_bytes > 0;
    }
    // Closing the Socket
    close(descriptor);
    // Closing the File
    outfile.close();
}
