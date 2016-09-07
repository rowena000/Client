/*
 ** client.c -- a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SVRPORT "55001"   //  55000
#define SVRNAME "dns.postel.org"  //localhost

#define COMMAND_X "X"
#define COMMAND_Y "Y"
#define COMMAND_Z "Z"
#define COMMAND_GO "GO"
#define COMMAND_STOP "STOP"

//define different states
#define S_APPLE "apple"
#define S_PEAR "pear"
#define S_CHERRY "cherry"
#define S_ORANGE "orange"
#define S_KIWI "kiwi"
#define S_LEMON "lemon"
#define S_LIME "lime"
#define S_GRAPE "grape"

#define MAXDATASIZE 1000
#define TIMEOUT 5 // SET timeout seconds

//////////////////////////////////////
// send a string ending in \n
//////////////////////////////////////
void mealysend(int sendsock, char *b, int bsize)
{
	char sendbuf[MAXDATASIZE + 3] = "";

	strncpy(sendbuf, b, MAXDATASIZE);
	strncat(sendbuf, "\n", MAXDATASIZE+2);

	if (send(sendsock,sendbuf,strlen(sendbuf), 0) == -1){
		perror("Client: send");
		exit(1);
	}
	printf("\tClient sent: %s",b);
	printf(" --  and it was %d bytes.\n",bsize);
	fflush(stdout);
}

//////////////////////////////////////
// receive up to the first \n
// (but omit the \n)
//  return 0 on EOF (socket closed)
//  return -1 on timeout
//////////////////////////////////////
int mealyrecvto(int recvsock, char *b, int bsize, int to)
{
	int num;
	int selectresult;
	fd_set myset;
	struct timeval tv;  // Time out

	int count = 0;
	char c = '\127';

	memset(b,0,MAXDATASIZE);

	while ((count < (bsize-2)) && (c != '\n') && (c != '\0')) {
		FD_ZERO(&myset);
		FD_SET(recvsock, &myset);
		tv.tv_sec = to;
		tv.tv_usec = 0;
		if ((to > 0) &&
				((selectresult = select(recvsock+1, &myset, NULL, NULL, &tv)) <= 0)) {
			// timeout happened (drop what you were waiting for -
			// if it was delimited by \n, it didn't come!
			return -1;
		}
		// got here because select returned >0, i.e., no timeout
		if ((num = recv(recvsock, &c, 1, 0)) == -1) {
			perror("Client: recv failed");
			exit(1);
		}
		if (num == 0) {
			// nothing left to read (socket closed)
			// no need to wait for a timeout; you're already done by now
			return 0;
		}
		b[count] = c;
		count++;
	}
	// at this point, either c is \n, \r or bsize has been reached
	// so just add a string terminator
	char *place;
	place = strchr(b,'\r');
	if (place != NULL) {
		*place = '\0';
	}
	place = strchr(b,'\n');
	if (place != NULL) {
		*place = '\0';
	}

	printf("\tClient received: %s: and it was %d bytes.\n",b,(int)strlen(b));
	return strlen(b);
}

void changeState(char *sfrom, char *sto, char *output) {
	printf("Changing state from \"%s\" to \"%s\". Output message is: %s.\n", sfrom, sto, output);
	strcpy(sfrom, sto);
}

void mealy(int msock)
{

	char state[10] = S_APPLE;
	char output[MAXDATASIZE] = "";
	char input[MAXDATASIZE] = "";
	int numbytes;
	char command[2];
	int to = TIMEOUT; // set timeout seconds

	while (1) {
		printf("Your current state is: \"%s\", ", state);
		if (strcmp(state, S_APPLE) == 0) {
			printf("Please input command \"GO\" to start \n");
		} else if (strcmp(state, S_CHERRY) == 0) {
			printf("please input command \"X\", \"Y\", \"Z\" to continue, \"STOP\" to exit \n");
		}
		else {
			printf("Something wrong!\n");
			break;
		}

		//ask user to type command;
		scanf("%s", command);
		printf("Your command is: %s. ", command);

		if (strcmp(command, COMMAND_STOP) == 0 && strcmp(state, S_CHERRY) == 0) {
			printf("Exit the program.\n");
			break;
		} else if (strcmp(command, COMMAND_GO) == 0 && strcmp(state, S_APPLE) == 0) {
			strcpy(output,"A");
			changeState(state, S_PEAR, output);
		} else if (strcmp(command, COMMAND_X) == 0 && strcmp(state, S_CHERRY) == 0) {
			strcpy(output,"B");
			changeState(state, S_ORANGE, output);
		} else if (strcmp(command, COMMAND_Y) == 0 && strcmp(state, S_CHERRY) == 0) {
			strcpy(output,"D");
			changeState(state, S_KIWI, output);
		} else if (strcmp(command, COMMAND_Z) == 0 && strcmp(state, S_CHERRY) == 0) {
			strcpy(output,"E");
			changeState(state, S_LEMON, output);
		} else {
			printf("Your command is incorrect! Please verify and type again.\n");
			*output = '\0';
		}

		//if there is nothing to send, it means we need user's command
		//if there is output message, send to server and try to get response
		while (*output != '\0') {
			mealysend(msock,output,strlen(output));
			numbytes = mealyrecvto(msock, input, MAXDATASIZE, to);
			//while timeout, resend to server
			while (numbytes == -1 && (strcmp(output, "E")==0||strcmp(output, "F")==0)) {
				printf("\t!Timeout! Retransmission message: %s, to server.\n", output);
				mealysend(msock,output,strlen(output));
				numbytes = mealyrecvto(msock, input, MAXDATASIZE, to);
			}

			printf("Input message is: %s. ", input);

			if (strcmp(input, "1") == 0) {
				//if receive 1 from server, change state to cherry
				//output message is null, do not send anything to server
				*output = '\0';
				changeState(state, S_CHERRY, output);
			} else if (strcmp(input, "2") == 0) {
				strcpy(output,"E");
				changeState(state, S_GRAPE, output);
			} else if (strcmp(input, "3") == 0) {
				//if receive 3 from server, change state to cherry, notify server, server won’t return
				// then prompt user's for command
				strcpy(output,"C");
				changeState(state, S_CHERRY, output);
				mealysend(msock,output,strlen(output));
				break;
			} else if (strcmp(input, "5") == 0 &&
					(strcmp(state, S_GRAPE) == 0 || strcmp(state, S_LIME) == 0)) {
				*output = '\0';
				changeState(state, S_APPLE, output);
			} else if (strcmp(input, "5") == 0 && strcmp(state, S_LEMON) == 0) {
				strcpy(output,"F");
				changeState(state, S_LIME, output);
			} else {
				//Incorrect message is sent to client
				printf("Client receive incorrect message. Break...\n");
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int sockfd, sen, valopt;

	struct hostent *hp;
	struct sockaddr_in server;
	socklen_t lon;

	// remove commments if your compile complains that these
	// are not defined properly:
	//  extern char *optarg;
	//  extern int optind;
	int c;
	char *sname=SVRNAME, *pname = SVRPORT;
	static char usage[] = "usage: %s [-s sname] [- p pname]\n";

	long arg;
	fd_set myset;
	struct timeval tv;  // Time out
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	// parse the command line arguments
	while ((c = getopt(argc, argv, "s:p:")) != -1)
		switch (c) {
		case 'p':
			pname = optarg;
			break;
		case 's':
			sname = optarg;
			break;
		case '?':
			fprintf(stderr, usage, argv[0]);
			exit(1);
			break;
		}

	// convert the port string into a network port number
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(pname));

	// convert the hostname to a network IP address
	hp = gethostbyname(sname);
	if ((hp = gethostbyname(sname)) == NULL) {  // get the host info
		herror("Client: gethostbyname failed");
		return 2;
	}
	bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
	bzero(&server.sin_zero, 8);

	// get a socket to play with
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
		perror("Client: could not create socket");
		exit(-1);
	}

	//////////////////////////////////////
	// open the connection

	// set the connection to non-blocking
	arg = fcntl(sockfd, F_GETFL, NULL);
	arg |= O_NONBLOCK;
	fcntl(sockfd, F_SETFL, arg);
	//
	if ((sen = connect(sockfd,(struct sockaddr *)&server,sizeof(server))) == -1) {
		if (errno == EINPROGRESS) {
			FD_ZERO(&myset);
			FD_SET(sockfd, &myset);
			if (select(sockfd + 1, NULL, &myset, NULL, &tv) > 0){
				lon = sizeof(int);
				getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
				if (valopt) {
					fprintf(stderr, "Client error in connection() %d - %s\n", valopt, strerror(valopt));
					exit(-1);
				}
			}
			else {
				perror("Client: connection time out");
				exit(-1);
			}
		}
		else {
			fprintf(stderr, "Client error connecting %d - %s\n", errno, strerror(errno));
			exit(0);
		}
	}
	//
	///////////////////////////////

	// Set to blocking mode again
	arg = fcntl(sockfd, F_GETFL, NULL);
	arg &= (~O_NONBLOCK);
	fcntl(sockfd, F_SETFL, arg);

	//////////////////////////////

	mealy(sockfd);
	// and now it's done
	//////////////////////////////

	close(sockfd);
	return 0;
}
