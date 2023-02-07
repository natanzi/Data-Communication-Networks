
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MESSAGE_SIZE 1024

struct sockaddr_in server_addr;

int create_socket(const char *server_address, int port_number) {
    // Creating the UDP Socket
    int descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    // Terminating the program if socket creation failed.
    if (descriptor < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Filling out the struct with null bytes.
    memset(&server_addr, 0, sizeof(server_addr));
    // Setting Socket Properties
    server_addr.sin_family = AF_INET; // server_addr family to communicate with
    inet_aton(server_address, &server_addr.sin_addr); // target ip address
    server_addr.sin_port = htons(port_number);
    // There's no "binding" step for the client.
    // Returning the Descriptor for Further Use
    return descriptor;
}

int main(int argc, char *argv[]) {
    // Command Line Arguments
    const char *server_ip = argv[1];
    int n_port = atoi(argv[2]);
    const char *filename = argv[3];
    // Creating the Initial Socket
    int descriptor = create_socket(server_ip, n_port);
    // Declaring the Necessary Variables for Communication
    socklen_t len = sizeof(server_addr);
    char buffer[MESSAGE_SIZE];
    ssize_t bytes_read, bytes_sent;
    // Initiating Handshake
    memset(buffer, 0, MESSAGE_SIZE);
    strcpy(buffer, "ABC");
    bytes_sent = sendto(descriptor, buffer, MESSAGE_SIZE, MSG_CONFIRM,
                        reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
    if (bytes_sent < 0) {  // sendto failed to send any bytes properly.
        perror("send failed");
        exit(EXIT_FAILURE);
    }
    // Processing the Server's Response
    memset(buffer, 0, MESSAGE_SIZE);
    bytes_read = recvfrom(descriptor, buffer, MESSAGE_SIZE, MSG_WAITALL,
                          reinterpret_cast<sockaddr *>(&server_addr), &len);
    if (bytes_read < 0) {  // recvfrom failed to receive any bytes properly.
        perror("receive failed");
        exit(EXIT_FAILURE);
    }
    int r_port = atoi(buffer);
    // Closing the Current Socket
    close(descriptor);
    // Opening the Target File
    std::ifstream infile(filename);
    // Waiting for the server to start listening.
    sleep(1);
    // Creating a New Socket
    descriptor = create_socket(server_ip, r_port);
    // Sending the Text in Chunks
    bool data_left = true;
    while (data_left) {
        // In our communication protocol, the first byte specifies the number of valid bytes in the chunk.
        // Since we acknowledge every single packet, we don't need to add any more info to maintain order.
        // We need to read from the file into the buffer, skipping the first byte.
        // Each chunk includes four characters.
        memset(buffer, 0, MESSAGE_SIZE);
        infile.read(buffer + 1, 4);
        // Saving the number of valid bytes in the first byte of the chunk.
        buffer[0] = static_cast<char>(infile.gcount());
        // Sending the Chunk
        bytes_sent = sendto(descriptor, buffer, MESSAGE_SIZE, MSG_CONFIRM,
                            reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
        if (bytes_sent < 0) {  // sendto failed to send any bytes properly.
            perror("send failed");
            exit(EXIT_FAILURE);
        }
        // Checking if there's any data left in the file.
        data_left = infile.gcount();
        // Waiting for acknowledgement.
        memset(buffer, 0, MESSAGE_SIZE);
        bytes_read = recvfrom(descriptor, buffer, MESSAGE_SIZE, MSG_WAITALL,
                              reinterpret_cast<sockaddr *>(&server_addr), &len);
        if (bytes_read < 0) {  // recvfrom failed to receive any bytes properly.
            perror("receive failed");
            exit(EXIT_FAILURE);
        }
        // Displaying the acknowledgement message on the screen.
        std::cout << buffer << std::endl;
    }
    // Closing the Socket
    close(descriptor);
    // Closing the File
    infile.close();
}
