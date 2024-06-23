#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXLINE 1023
#define FILENAME_MAXLEN 256

typedef struct {
    char username[20];
    char password[20];
} User;

User users[] = {
    {"Jay", "0214"},
    {"Sunny", "0606"},
    {"Choi", "0421"},
};

int authenticate_user(const char *username, const char *password) {
    for (int i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            printf("Connection Successful. You can access this system!\n");
            return 1; // successful
        }
    }
    printf("You cannot access this system.\n");
    return 0; // failed
}

void handle_put_command(const char *filename, const char *userDir, int recv_s) {
    char filePath[FILENAME_MAXLEN];
    FILE *file;

    // 파일 경로 생성
    sprintf(filePath, "%s%s", userDir, filename);

    // 파일 열기
    if ((file = fopen(filePath, "wb")) == NULL) {
        perror("error: fopen");
        exit(0);
    }

    // 파일 데이터 수신 및 저장
    char buffer[MAXLINE];
    int n;
    struct sockaddr_in from;
    int addrlen = sizeof(struct sockaddr_in);
    while ((n = recvfrom(recv_s, buffer, MAXLINE, 0, (struct sockaddr *)&from, &addrlen)) > 0) {
        fwrite(buffer, 1, n, file);
    }

    // 파일 닫기
    fclose(file);

    printf("File '%s' received and saved in '%s'\n", filename, userDir);
}

int main(int argc, char *argv[]) {
    int send_s, recv_s, n, len;
    pid_t pid;
    unsigned int yes = 1;
    struct sockaddr_in mcast_group;
    struct ip_mreq mreq;
    char name[20];  // Increase buffer size for username
    char password[20];  // Increase buffer size for password

    if (argc != 5) {  // Correct the number of command line arguments
        printf("usage: %s multicast_address port My_name Password\n", argv[0]);
        exit(0);
    }

    strcpy(name, argv[3]);
    strcpy(password, argv[4]);  // Pass the password as an argument

    // 사용자 이름을 기반으로 디렉토리 생성
    char userDir[FILENAME_MAXLEN];
    sprintf(userDir, "./%s/", name);

    // 파일 경로 생성
    char filePath[FILENAME_MAXLEN];

    // 사용자 인증 체크
    if (authenticate_user(name, password)) {
        // 디렉토리 생성
        if (access(userDir, F_OK) == -1) {
            // 디렉토리가 존재하지 않으면 생성
            if (mkdir(userDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
                perror("error: mkdir");
                exit(0);
            }
        }
    } else {
        printf("You are not USERS. Directory not created.\n");
        exit(0);
    }

    memset(&mcast_group, 0, sizeof(mcast_group));
    mcast_group.sin_family = AF_INET;
    mcast_group.sin_port = htons(atoi(argv[2]));

    // Add error handling for inet_pton
    if (inet_pton(AF_INET, argv[1], &mcast_group.sin_addr) <= 0) {
        perror("error: inet_pton");
        exit(0);
    }

    if ((recv_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error: Cannot create receive socket");
        exit(0);
    }

    mreq.imr_multiaddr = mcast_group.sin_addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(recv_s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("error: add membership");
        exit(0);
    }

    if (setsockopt(recv_s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("error: reuse setsockopt");
        exit(0);
    }

    if (bind(recv_s, (struct sockaddr *)&mcast_group, sizeof(mcast_group)) < 0) {
        perror("error: bind receive socket");
        exit(0);
    }

    if ((send_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error: Cannot create send socket");
        exit(0);
    } else if ((pid = fork()) == 0) {
        struct sockaddr_in from;
        char message[MAXLINE + 1];
        int addrlen = sizeof(struct sockaddr_in);
        for (;;) {
            printf("receiving msg ...\n");
            if ((n = recvfrom(recv_s, message, MAXLINE, 0, (struct sockaddr *)&from, &addrlen)) < 0) {
                perror("error: recvfrom");
                exit(0);
            }
            message[n] = 0;
            printf("Received Message: %s\n", message);
        }
    } else {
        char message[MAXLINE + 1], line[MAXLINE + 1];
        printf("Send message: ");
        while (fgets(message, MAXLINE, stdin) != NULL) {
            // before sending the message
            if (authenticate_user(name, password)) {
                sscanf(message, "%s", line);
                char command[5], filename[FILENAME_MAXLEN];
                if (sscanf(message, "%s %s", command, filename) == 2) {
                    // 'ls' command
                    if (strcmp(command, "ls") == 0) {
                        printf("Listing files in directory: %s\n", userDir);
                        continue; // Skip sending 'ls' command to the server
                    }

                    // 'get' and 'put' commands
                    if (strcmp(command, "get") == 0) {
                        // 'get' 명령을 서버에 전송
                        len = strlen(message);
                        if (sendto(send_s, message, len, 0, (struct sockaddr *)&mcast_group, sizeof(mcast_group)) < len) {
                            perror("error: sendto");
                            exit(0);
                        }
                    } else if (strcmp(command, "put") == 0) {
                        // 'put' 명령을 서버에 전송
                        len = strlen(message);
                        if (sendto(send_s, message, len, 0, (struct sockaddr *)&mcast_group, sizeof(mcast_group)) < len) {
                            perror("error: sendto");
                            exit(0);
                        }
                        handle_put_command(filename, userDir, recv_s);
                    } else {
                        printf("Invalid command.\n");
                        continue;
                    }

                    // Save the file on the client side
                    FILE *file = fopen(filePath, "w");
                    if (file == NULL) {
                        perror("error: fopen");
                        exit(0);
                    }
                    fprintf(file, "%s", message);
                    fclose(file);
                } else {
                    len = strlen(message);
                    if (sendto(send_s, message, len, 0, (struct sockaddr *)&mcast_group, sizeof(mcast_group)) < len) {
                        perror("error: sendto");
                        exit(0);
                    }
                }
            } else {
                printf("You are not USERS. Message not sent.\n");
            }
        }
    }

    return 0;
}
