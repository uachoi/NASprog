#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1023

typedef struct {
    char username[20];
    char password[20];
} User;

User users[] = {
    {"Jay", "0214"},
    {"Sunny", "0606"},
    {"Choi", "0421"},
    // Add more users as needed
};

int authenticate_user(const char *username, const char *password) {
    for (int i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return 1; // Authentication successful
        }
    }
    return 0; // Authentication failed
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
            // Authenticate the user before sending the message
            if (authenticate_user(name, password)) {
                sprintf(line, "%s %s", name, message);
                len = strlen(line);
                if (sendto(send_s, line, strlen(line), 0, (struct sockaddr *)&mcast_group, sizeof(mcast_group)) < len) {
                    perror("error: sendto");
                    exit(0);
                }
            } else {
                printf("Authentication failed. Message not sent.\n");
            }
        }
    }

    return 0;
}


=================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define MAXLINE 1023

int main(int argc, char *argv[]) {
	int send_s, recv_s, n, len; 
	pid_t pid;
	unsigned int yes=1;
	struct sockaddr_in mcast_group;
	struct ip_mreq mreq;
	char name[10];
	
	if (argc !=4) {
		printf("using : %s multicast_address port My_name Password \n",argv[0]);
		exit(0);
	}
	sprintf(name, "[%s]", argv[3]);
	
	memset(&mcast_group,0,sizeof(mcast_group));
	mcast_group.sin_family=AF_INET;
	mcast_group.sin_port=htons(atoi(argv[2]));
	inet_pton(AF_INET,argv[1],&mcast_group.sin_addr);
	if( (recv_s=socket(AF_INET,SOCK_DGRAM,0))<0) {
		printf("error: Cannor create receive socket \n");
		exit(0);
	}
	
	mreq.imr_multiaddr=mcast_group.sin_addr;
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if(setsockopt(recv_s,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq))<0){
		printf("error: add membership \n");
		exit(0);
	}
	
	if(setsockopt(recv_s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))<0) {
		printf("error:reuse setsockopt \n");
		exit(0);
	}
	
	if(bind(recv_s, (struct sockaddr*)&mcast_group,sizeof(mcast_group))<0) {
		printf("error: bind receive socket\n");
		exit(0);
	}
	
	if((send_s=socket(AF_INET, SOCK_DGRAM,0))<0) {
		printf("error: Cannot create send socket\n");
		exit(0);
	}
	else if (pid==0) {
		struct sockaddr_in from;
		char message[MAXLINE+1];
		int addrlen=sizeof(struct sockaddr_in);
		for(;;) {
			printf("receiving msg ...\n");
			//len=sizeof(from);
			if ((n=recvfrom(recv_s,message,MAXLINE,0,(struct sockaddr*)&from,&addrlen))<0) {
				printf("error:recvfrom\n");
				exit(0);
			}
		message[n]=0;
		printf("Receiced Message: %s\n", message);
		}
	}
	else {
	char message[MAXLINE+1],line[MAXLINE+1];
	printf("Send message: ");
	while (fgets(message,MAXLINE, stdin) != NULL) {
		sprintf(line, "%s %s", name, message);
		len=strlen(line);
		if(sendto(send_s, line, strlen(line),0,(struct sockaddr*)&mcast_group,sizeof(mcast_group))<len) {
			printf("error:sendto \n");
			exit(0);
			}
		}
	}
	return 0;
}
			
