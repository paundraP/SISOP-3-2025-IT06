#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 16284
#define FILE_SIZE 256
#define DATABASE "server/database"
#define I_HOPE_OUR_PATH_CROSS_AGAIN "/Users/paundrapujodarmawan/Documents/KULIAH/SEMESTER 2/SISOP/Sisop-3-2025-IT06/soal_1"

// error
const char *file_not_found = "File not found";
const char *error_save_file = "Failed to save file";
const char *invalid_format = "Format is invalid";
const char *invalid_command = "Unknown command";

// success
const char *success_decode = "Successfully decrypt the input";

// info 
const char *exiting_connection = "Client requested to exit";
const char *cant_reach_server = "Client cant reach the server";

// action
const char *error = "ERROR";
const char *decrypt = "DECRYPT";
const char *download = "DOWNLOAD";
const char *upload = "UPLOAD";
const char *exit_rpc = "EXIT";
const char *save = "SAVE";

//source
const char *client = "Client";
const char *server = "Server";

// untuk reverse string
void reverse(char *str){
    int len = strlen(str);
    for(int i = 0; i < len/2; i++) {
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
    }
}

// untuk decode file yang sudah di reverse
int decode(const char *hex, unsigned char **res) {
    int len = strlen(hex);
    if (len % 2 != 0) return -1; // panjang hex invalid 

    *res = malloc(len / 2);
    if (!*res) return -1;

    for (int i = 0; i < len; i += 2) {
        sscanf(hex + i, "%2hhx", &((*res)[i / 2]));
    }

    return len / 2; // ngembaliin panjang binary data
}


// write log
void write_log(const char *source, const char *action, const char *info) {
    FILE *log = fopen("server/server.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
    fclose(log);
}

// mendapatkan timestamp
time_t get_timestamp() {
    return time(NULL);
}

// process untuk daemonize / berjalan di background
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    umask(0);
    setsid();
    chdir(I_HOPE_OUR_PATH_CROSS_AGAIN);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// service untuk melakukan decrypt terhadap file yang telah dikirim
void decrypt_service(int client_socket, const char *data) {
    char *tmp = strdup(data);
    reverse(tmp);


    unsigned char *decoded = NULL;
    int decoded_len = decode(tmp, &decoded);
    free(tmp);

    time_t timestamp = get_timestamp();
    char path[FILE_SIZE];
    snprintf(path, sizeof(path), "%s/%ld.jpeg", DATABASE, timestamp);

    FILE *fp = fopen(path, "wb");
    if (fp) {
        fwrite(decoded, 1, decoded_len, fp);
        fclose(fp);
        send(client_socket, data, strlen(data), 0);

        write_log(client, decrypt, "Text Data");
        char info[FILE_SIZE];
        snprintf(info, sizeof(info), "%ld.jpeg", timestamp);
        write_log(server, save, info);
    } else {
        send(client_socket, error_save_file, strlen(error_save_file), 0);
    }
}

// service untuk melakukan download/transfer file dari server ke client
void download_service(int client_socket, const char *filename) {
    char path[FILE_SIZE];
    snprintf(path, sizeof(path), "%s/%s", DATABASE, filename);

    FILE *fp = fopen(path, "rb");
    if (fp) {
        char buffer[BUFFER_SIZE];
        size_t n;
        while((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            send(client_socket, buffer, n, 0);
        }
        fclose(fp);
        write_log(client, download, filename);
        write_log(server, upload, filename);
    } else {
        send(client_socket, file_not_found, strlen(file_not_found), 0);
        write_log(server, error, file_not_found);
    }
}


// handle request yang dikirim client
void client_handler(int client_socket) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (recv_len <= 0) {
            write_log(client, exit_rpc, cant_reach_server);
            break;
        }

        char *cmd = strtok(buffer, "::");
        char *content = buffer + strlen(cmd) + 2; // +2 buat escape "::"
        // Cari kalau ada marker <EOF>
        char *eof_marker = strstr(content, "<EOF>");
        if (eof_marker) {
            *eof_marker = '\0';
        }

        // Cari kalau ada trailing "::" di akhir data
        char *last_delim = strstr(content, "::");

        // Kalau ada di remove
        if (last_delim && *(last_delim + 2) == '\0') {
            *last_delim = '\0';
        }
        if (!cmd) continue;
        if (strcmp(cmd, decrypt) == 0) {
            if (strlen(content) > 0) {
                decrypt_service(client_socket, content);
            } else {
                send(client_socket, invalid_format, strlen(invalid_format), 0);
            }
        } else if (strcmp(cmd, download) == 0) {
            char *filename;
            strcpy(filename, content);
            if (filename) {
                download_service(client_socket, filename);
            } else {
                send(client_socket, file_not_found, strlen(file_not_found), 0);
            }
        } else if (strcmp(cmd, "EXIT") == 0) {
            write_log(client, exit_rpc, exiting_connection);
            break;
        } else {
            send(client_socket, invalid_command, strlen(invalid_command), 0);
        }
    }

    close(client_socket);
}

int main() {
    daemonize();

    int server_fd, client_socket;
    struct sockaddr_in addr;
    int opt = 1;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 3);

    while(1) {
        client_socket = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_socket < 0) continue;

        if (fork() == 0) {
            close(server_fd);
            client_handler(client_socket);

            write_log(client, exit_rpc, exiting_connection);
            close(client_socket);
            exit(0);
        }
        close(client_socket);
    }

    return 0;
}
