#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 8192
#define FILE_SIZE 256
#define SECRET_PATH "client/secrets/"
#define DOWNLOAD_PATH "client/"


void menu() {
    printf("IMAGE DECODER CLIENT\n");
    printf("1. Send file to server\n");
    printf("2. Download decoded file from server\n");
    printf("3. Exit\n");
    printf(">> ");
}

void upload_handler(int socket_fd) {
    char filename[FILE_SIZE];
    char path[FILE_SIZE];
    printf("Enter filename (ex: input_1.txt): ");
    scanf("%s", filename);
    snprintf(path, sizeof(path), "%s%s", SECRET_PATH, filename);

    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Failed to read the file inputed");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);

    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "DECRYPT::%s::", content);
    send(socket_fd, header, strlen(header), 0);
    
    char buffer[BUFFER_SIZE];
    int recv_len = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (recv_len > 0) {
        buffer[recv_len] = '\0';
        printf("Server Response: %s\n", buffer);
    } else {
        printf("Failed to receive server response\n");
    }

    free(content);
}


void download_handler(int socket_fd) {
    char filename[FILE_SIZE];
    printf("Enter filename (ex: 1744403652.jpeg): ");
    scanf("%s", filename);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "DOWNLOAD::%s<EOF>", filename);
    send(socket_fd, buffer, strlen(buffer), 0);

    FILE *out = NULL;
    snprintf(buffer, sizeof(buffer), "%s%s", DOWNLOAD_PATH, filename);
    out = fopen(buffer, "wb");
    if (!out) {
        perror("Failed to create file");
        return;
    }
    
    char recv_buf[BUFFER_SIZE];
    int recv_len = recv(socket_fd, recv_buf, sizeof(recv_buf), 0);
    if (recv_len <= 14) {
        perror("Failed to download file");
        return;
    }
    fwrite(recv_buf, 1, recv_len, out);
    fclose(out);
    printf("File successfuly downloaded to %s\n", buffer);
}

int main() {
    struct sockaddr_in server_addr;
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    int choice;
    while (1) {
        menu();
        scanf("%d", &choice);
        getchar();
        switch (choice) {
            case 1:
                upload_handler(socket_fd);
                break;
            case 2:
                download_handler(socket_fd);
                break;
            case 3:
                printf("Exiting...\n");
                send(socket_fd, "EXIT::\n", 8, 0);
                close(socket_fd);
                exit(EXIT_SUCCESS);
            default:
                printf("Choose valid option!\n");
        }
    }

    return 0;
}