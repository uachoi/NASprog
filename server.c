#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAXLINE 1023
#define FILENAME_MAXLEN 256


void list_received_files(const char *directory) {
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(directory)) != NULL) {
        printf("Files in directory '%s':\n", directory);
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) { // Only regular files
                printf("%s\n", ent->d_name);
            }
        }
        closedir(dir);
    } else {
        perror("error: opendir");
    }
}

int main(int argc, char *argv[]) {
    int recv_s, n;
    unsigned int yes = 1;
    struct sockaddr_in mcast_group, from;
    struct ip_mreq mreq;

    if (argc != 3) {
        printf("Usage: %s multicast_address port\n", argv[0]);
        exit(0);
    }

    memset(&mcast_group, 0, sizeof(mcast_group));
    mcast_group.sin_family = AF_INET;
    mcast_group.sin_port = htons(atoi(argv[2]));

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

    char received_message[MAXLINE + 1];
    socklen_t addrlen = sizeof(struct sockaddr_in);

    for (;;) {
        printf("Waiting for a file...\n");
        if ((n = recvfrom(recv_s, received_message, MAXLINE, 0, (struct sockaddr *)&from, &addrlen)) < 0) {
            perror("error: recvfrom");
            exit(0);
        }
        received_message[n] = 0;

        // Assume the received message contains file content
        // You may need to add more checks or protocols for better handling
        FILE *file = fopen("server_received_file.txt", "wb");
        if (file == NULL) {
            perror("error: fopen");
            exit(0);
        }
        //fprintf(file, "%s", received_message);
	fwrite(received_message, 1, n, file);
        fclose(file);

        printf("File received and saved as 'server_received_file.txt'\n");
	list_received_files(".");
    }

    return 0;
}
