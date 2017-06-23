#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define MAX_LEN (1024)
#define DEFAULT_PORT (6423)
#define TOTAL_TO (20)
#define MAXMAILS (32000)
#define MAX_USERNAME (50)
#define MAX_PASSWORD (50)
#define MAX_SUBJECT (100)
#define MAX_CONTENT (2000)
#define NUM_OF_CLIENTS (20)

/*
 a struct representing an email. each user will have an array of these.
 */
typedef struct EMAIL
{
    //mail id is the place in the array of the mail
    char from[MAX_CONTENT];
    char to[MAX_CONTENT];
    char subject[MAX_CONTENT];
    char content[MAX_CONTENT];
    bool deleted;
} email;

/*
 * main struct for the database, array of USERDB, each user has his own USERDB.
 */
typedef struct USERDB
{
    char name[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int numOfEmails;
    email emails[MAXMAILS];
    int isActivated;
    int userIndexInClientHandler;
} userDB;

/*
 * the struct for handling multiple connections. holds data for who is online, 
 * his file descriptor, 
 * his index in DB
 */

typedef struct clientHandler_t{
    int fd;
    int confirmedUser;
    int isActivated;
    char* username;
    int userIndexInDB;
} clientHandler;

/**
 * gets the user 'username' index in the database pointed by userdb
 * @param userdb - pointer to the database
 * @param userCnt - number of users in db
 * @param username - the username to check the index of him in database
 * @return 
 */
int getUserIndexInDB(userDB* userdb,int userCnt,char* username);
/**
 * checks that the string s contains only num chars
 * @param s - the string to check
 * @return true if only num chars. else false
 */
bool onlyNumbers(char* s);
/*
 * sends all data in buf. learned in class.
 * return -1 on ERROR 0 on success
 */
int sendall (int s, char *buf, int len);
/*
 *receives all data. learned in class
 * return -1 on ERROR 0 on success
 */
int recvall(int s, char *buf, int len);
/**
 * validates the user cardentials inserted
 * @param filePath - path to DB file to compare
 * @param userCred - the user credntials
 * @return - true if ok, false if not equal to database
 */
bool validateUserCred(char* filePath,char* userCred);
/**
 * parsing user database at boot of server
 * @param filePath - path to DB.txt file
 * @param userCnt
 * @return NULL on error, pointer to UserDb filled with boot data on success.
 */
userDB* parseUserDB(char** filePath, int* userCnt);
/*
 * returns number of users on data.txt
 */
int countUsers(char** filePath);
/*
 * a handler for SHOW_INBOX request
 */
void printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb);
/*
 * a handler for GET_MAIL * user request
 */
void printEmailById(int emailId, int userIndexInDB, int clientSocket, userDB* userdb);

/*
 * a handler for DELETE_MAIL * user request
 */

void deleteEmailById(int emailId, int userIndexInDB, userDB* userdb);
/*
 * insert the mail given in composeData into the userDB
 * returns -1 on error 0 on success
 */
int insertEmailToDB(char* username, int userCount, int clientSocket, userDB* userdb,char* composeData);
/*
 * function makes sure all the message inside buf will be received using
 * recvall learned in class. must have sendFullMsg on the other side sending the message
 * returns: -1 on error. 0 on success.
 */
int recvFullMsg(int sock, char* buf);
/*
 * function makes sure all the message inside buf will be sent to the other side using
 * sendall learned in class. must have recvFullMsg on the other side accepting the message
 * returns: -1 on error. 0 on success.
 */
int sendFullMsg(int sock, char* buf);
/*
 * sub function of recvFullMsg
 * receives the length of the buffer before receiving the data itself
 * return -1 on ERROR 0 on success
 */
int recvLenOfMsg(int sock, int *len);
/*
 * sub function of sendFullMsg
 * sends the length of the buffer before sending the data itself
 * return -1 on ERROR 0 on success
 */
int sendLenOfMsg(int sock, int len);
/*
 * max function. returns max from the two numbers.
 */
int max(int num1, int num2);
/*
 * the main function that handels all client requests using all handler functions mentioned in this file.
 */
int HandleClientMessage(userDB* userdb, clientHandler* fdCli, fd_set* set, int userCount, clientHandler* fdArray, int serverSocket, int* maxFdOfClients);
/*
 * closing and free all data for client. making sure he is marked as offline
 */
void closeClient(clientHandler* fdCli, clientHandler* fdArray, userDB* userdb);
/*
 * confirms username and password at server side
 * @param clientSocket - socket from which to receive the login
 * @param DBPath - path to DB to check login data
 * @param username - the username of the user
 * @return -1 on error, 0 on success - means valid user
 */
int confirmUser(int clientSocket, char* DBPath, char** username);
/**
 * a handler function to take care of SHOW_ONLINE_USERS request
 * @param fdArray the fdArray described above
 * @param fdCli - the socket of the client
 * @param userdb - the path to user database
 * @param userCount - how many users connected
 * @return  -1 error, else 0
 */
int showOnlineUsers(clientHandler* fdArray,clientHandler* fdCli, userDB* userdb, int userCount);
/**
 * a handler for MSG request from client
 * @param fdCli the socket of the client
 * @param fdArray the fdArray described above
 * @param userdb the path to user database
 * @param actualString - the string request
 * @param userCount - how many users connected
 * @return -1 ERROR, else 0
 */
int requestForSendingChatMsg(clientHandler* fdCli, clientHandler* fdArray, userDB* userdb, char* actualString, int userCount);
/**
 *  insets an email into the user database 
 * @param sender the sender of the mail
 * @param recipient 
 * @param userCount - how many users are there
 * @param userdb the pointer to the user database
 * @param actualString - the mail itself
 * @return -1 on error, 0 on success
 */
int insertChatMsgToDB(char* sender, char* recipient, int userCount,userDB* userdb,char* actualString);



int updateMaxFd(clientHandler* fdArray, int serverSocket){
    int newMax = 0;
    for(int i=0; i < NUM_OF_CLIENTS; i++){
        if(fdArray[i].isActivated){
            if(fdArray[i].fd > newMax){
                newMax = fdArray[i].fd;
            }
        }
    }
    return max(newMax, serverSocket) + 1;
}

int main(int argc, char** argv)
{
    int bindCheck,listenCheck;
    int check;
    struct sockaddr_in sockAddr;
    int userCount; // allocating inside function parseUserDB
    int clientSocket;

    if (argc != 2 && argc != 3)
    {
        printf("Incorrect parameters, try again.\n");
        return -1;
    }
    if (argc==3 && !onlyNumbers(argv[2]))
    {
        printf("Incorrect port number, try again.\n");
        return -1;
    }


    userDB* userdb = parseUserDB(&argv[1],&userCount);

    int serverSocket = socket(AF_INET,SOCK_STREAM,0);
    if (serverSocket <0)
    {
        printf("Error on opening socket: %s\n",strerror(errno));
        return -1;
    }


    sockAddr.sin_family = AF_INET; //Was AF_INET
    if (argc==3)
    {
        sockAddr.sin_port = htons(atoi(argv[2]));
    }
    else
    {
        sockAddr.sin_port = htons(DEFAULT_PORT);
    }

    inet_aton("127.0.0.1", &sockAddr.sin_addr);

    bindCheck = bind(serverSocket,(struct sockaddr*) &sockAddr,sizeof(sockAddr));
    if (bindCheck <0)
    {
        printf("Error on binding socket: %s\n",strerror(errno));
        return -1;
    }

    listenCheck = listen(serverSocket,NUM_OF_CLIENTS);
    if (listenCheck <0)
    {
        printf("Error listening to socket: %s\n",strerror(errno));
        return -1;
    }
    struct sockaddr_in clientAddr = {0};

    socklen_t clientLen = sizeof(sockAddr);

    //create array
    clientHandler fdArray[NUM_OF_CLIENTS];
    for(int i=0;i<NUM_OF_CLIENTS;i++){ //initialize
        fdArray[i].isActivated = 0;
        fdArray[i].confirmedUser = 0;
        fdArray[i].userIndexInDB = -1;
    }

    for(int i=0; i< userCount; i++){
        userdb[i].isActivated = 0;
        userdb[i].userIndexInClientHandler = -1;
    }

    //create fd_set
    fd_set set;
    int maxFdOfClients = serverSocket+1;
    int ResOfSelect = 0;
    FD_ZERO(&set);
    FD_SET(serverSocket, &set);

    struct timeval time;
    time.tv_sec = 0;
    time.tv_usec = 0;


    while(1)
    {
        for(int i=0; i < NUM_OF_CLIENTS; i++){
            if(fdArray[i].isActivated){ //insert to first available user
                FD_SET(fdArray[i].fd, &set);
            }
        }
        FD_SET(serverSocket,&set);

        maxFdOfClients = updateMaxFd(fdArray, serverSocket);
        ResOfSelect = select(maxFdOfClients, &set, NULL, NULL, NULL);  
        if(ResOfSelect < 0){
            printf("Error in select method: %s\n", strerror(errno));
            exit(-1);
        }

        //no information to process
        if(ResOfSelect == 0){
            for(int i=0; i < NUM_OF_CLIENTS; i++){
                if(fdArray[i].isActivated){ //insert to first available user
                    FD_SET(fdArray[i].fd, &set);
                }
            }
            FD_SET(serverSocket,&set);
            continue;
        }

        //if client is trying to connect - accept it
        if(FD_ISSET(serverSocket, &set)){
            clientSocket = accept(serverSocket,(struct sockaddr*) &clientAddr, &clientLen);
            if (clientSocket < 0)
            {
                printf("Error accepting client socket: %s\n",strerror(errno));
                for(int i=0; i < NUM_OF_CLIENTS; i++){
                    if(fdArray[i].isActivated){ //insert to first available user
                        FD_SET(fdArray[i].fd, &set);
                    }
                }
                FD_SET(serverSocket,&set);
                continue; 
            }
            check = sendFullMsg(clientSocket,"Welcome! I am simple-mail-server.");
            if (check < 0)
            {
                printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
                close(clientSocket);
                for(int i=0; i < NUM_OF_CLIENTS; i++){
                    if(fdArray[i].isActivated){ //insert to first available user
                        FD_SET(fdArray[i].fd, &set);
                    }
                }
                FD_SET(serverSocket,&set);
                continue; 
            }

            FD_SET(clientSocket, &set);
            maxFdOfClients = max(maxFdOfClients, clientSocket) +1; 
            int insertedToSlot = 0; 
            for(int i=0; i < NUM_OF_CLIENTS; i++){
                if(!insertedToSlot) {
                    if(!fdArray[i].isActivated){ //insert to first available user
                        fdArray[i].fd = clientSocket;
                        fdArray[i].confirmedUser = 0;
                        fdArray[i].isActivated = 1;
                        break;
                    }
                }
                if(fdArray[i].isActivated){
                    FD_SET(fdArray[i].fd, &set);
                }
            }
            continue;
        }


        for(int i=0; i<NUM_OF_CLIENTS; i++){
            if(fdArray[i].isActivated){
                if(FD_ISSET(fdArray[i].fd, &set)){
                    if(fdArray[i].confirmedUser){
                        if(HandleClientMessage(userdb, fdArray+i, &set, userCount, fdArray, serverSocket, &maxFdOfClients) < 0){ //closing and updating of set is done inside this function.
                            closeClient(fdArray+i, fdArray, userdb); 
                       }
                    }
                    else{
                        char* username = (char*)malloc(sizeof(char)*MAX_USERNAME);
                        fdArray[i].username = username; //set for free in case that is not correct.
                        if(confirmUser(fdArray[i].fd, argv[1], &username) < 0){
                            FD_CLR(fdArray[i].fd,&set);
                            closeClient(fdArray+i, fdArray, userdb);
                            fdArray[i].isActivated = 0; 
                        }
                        else{
                            fdArray[i].confirmedUser = 1;
                            fdArray[i].username = username;
                            fdArray[i].userIndexInDB = getUserIndexInDB(userdb, userCount, username);
                            userdb[fdArray[i].userIndexInDB].isActivated = 1;
                            userdb[fdArray[i].userIndexInDB].userIndexInClientHandler = i;
                        }

                    }
                }
                if(fdArray[i].isActivated){
                    FD_SET(fdArray[i].fd, &set);
                }
                else{
                    FD_CLR(fdArray[i].fd, &set);
                }
            }
        }
    }
    free(userdb); //TODO - how we release this?!
    close(serverSocket); //TODO - how we release this?!
    return 0;
}

void closeClient(clientHandler* fdCli, clientHandler* fdArray, userDB* userdb){
    if(fdCli->confirmedUser){
       fdCli->isActivated = 0;
       userdb[fdCli->userIndexInDB].isActivated = 0;
       userdb[fdCli->userIndexInDB].userIndexInClientHandler = -1;
    }

    fdCli->confirmedUser = 0;
    fdCli->userIndexInDB = -1;
//    if(fdCli->fd == *maxFdOfClients){
//        *maxFdOfClients = updateMaxFd(fdArray, serverSocket);
//    }
    free(fdCli->username);
    close(fdCli->fd);
}


int HandleClientMessage(userDB* userdb, clientHandler* fdCli, fd_set* set, int userCount, clientHandler* fdArray, int serverSocket, int* maxFdOfClients){

    const char delim[2] = " ";
    char* msg = (char*)malloc(sizeof(char)*MAX_LEN);
    char* actualString;

    int check = recvFullMsg(fdCli->fd,msg);
    if (check <0)
    {
        printf("Error receiving client message. \nDisconnecting Client.\n");
        free(msg);
        return -1;
    }
    char* userCmd = strtok(msg, delim);
    if(!strcmp(userCmd,"QUIT")){
        closeClient(fdCli,fdArray, userdb); //close the client here and return success value
        free(msg);
        return 0;
    }
    else if(!strcmp(userCmd,"COMPOSE")){
        actualString = (msg+8);
    }

    else if(!strcmp(userCmd,"MSG")){
        actualString = (msg+4);
    }


    int emailId;
    int userIndex = getUserIndexInDB(userdb,userCount,fdCli->username);

    if(!strcmp(userCmd,"SHOW_INBOX"))
    {
        printEmails(fdCli->username,userIndex,fdCli->fd,userdb);
    }
    else if(!strcmp(userCmd,"GET_MAIL"))
    {
        emailId = atoi(strtok(NULL, delim)); // turn into number the next word in string
        if(userdb[userIndex].emails[emailId-1].deleted)
        {
            check = sendFullMsg(fdCli->fd,"FAIL");
            if (check <0)
            {
                printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
                free(msg);
                return -1;
            }
        }
        else
        {
            check = sendFullMsg(fdCli->fd,"SUCCESS");
            if (check <0)
            {
                printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
                free(msg);
                return -1;
            }
            printEmailById(emailId,userIndex,fdCli->fd,userdb);
        }
    }
    else if(!strcmp(userCmd,"DELETE_MAIL"))
    {
        emailId = atoi(strtok(NULL, delim)); // turn into number the next word in string
        if(userdb[userIndex].emails[emailId-1].deleted)
        {
            check = sendFullMsg(fdCli->fd,"FAIL");
            if (check <0)
            {
                printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
                free(msg);
                return -1;
            }
        }
        else
        {
            deleteEmailById(emailId,userIndex,userdb);
            check = sendFullMsg(fdCli->fd,"SUCCESS");
            if (check <0)
            {
                printf("Error receiving client message: %s\nDisconnecting Client.\n",strerror(errno));
                free(msg);
                return -1;
            }
        }
    }
    else if(!strcmp(userCmd,"COMPOSE"))
    {
        if(insertEmailToDB(fdCli->username, userCount, fdCli->fd, userdb,actualString) == -1) 
        {
            printf("Error inserting email into DB. Disconnecting client.\n");
            free(msg);
            return -1;
        }
    }
    else if(!strcmp(userCmd,"SHOW_ONLINE_USERS"))
    {
        if(showOnlineUsers(fdArray,fdCli, userdb, userCount) < 0){
            free(msg);
            return -1;
        }
    }
    else if(!strcmp(userCmd,"MSG"))
    {
        if(requestForSendingChatMsg(fdCli, fdArray, userdb, actualString, userCount) < 0){
            free(msg);
            return -1;
        }
    }

    free(msg);
    return 0;
}

int sendFullMsg(int sock, char* buf)
{
    int lenSent = strlen(buf)+1;
    int sendCheck = sendLenOfMsg(sock,lenSent);
    if (sendCheck <0)
        {
            printf("Error sending client message size: %s\n",strerror(errno));
            return -1;
        }

    sendCheck = sendall(sock,buf,lenSent);
    if (sendCheck <0)
        {
            printf("Error sending client message: %s\n",strerror(errno));
            return -1;
        }
    return 0;
}

int recvFullMsg(int sock, char* buf)
{
    int recvLen = 0;
	if (recvLenOfMsg(sock, &recvLen) != 0) return -1;
	if (recvall(sock, buf, recvLen) != 0) return -1;

	return 0;
}

int sendLenOfMsg(int sock, int len)
{
	if (send(sock, &len, sizeof( len ),0) != 4)
	{
		return -1;
	}
	return 0;
}

int recvLenOfMsg(int sock, int *len)
{
    if (recv(sock, len, sizeof(*len), 0) != 4)
    {
     	return -1;
    }
    return 0;
}

int recvall(int s, char *buf, int len)
{
	//total = how many bytes we've sent, bytesleft =  how many we have left to send
	int total = 0, n, bytesleft = len;

	while(total < len)
	{
		n = recv(s, buf+total, bytesleft, 0);
		if ((n == -1) || (n == 0))
		{
			return -1;
		}
		total += n;
		bytesleft -= n;
	}
	//-1 on failure, 0 on success
	return 0;
}

int sendall(int s, char *buf, int len)
{
	/* total = how many bytes we've sent, bytesleft =  how many we have left to send */
	int total = 0, n, bytesleft = len;

	while(total < len)
	{
		n = send(s, buf+total, bytesleft, MSG_NOSIGNAL);
		if (n == -1)
		{
			break;
		}
		total += n;
		bytesleft -= n;
	}
	/*-1 on failure, 0 on success */
	return n == 1 ? -1:0;
}

bool onlyNumbers(char* s){
    int i=0;
    int len = strlen(s);
    char temp;
    for(i=0; i<len; i++)
    {
        temp = *(s+i);
        if(!isdigit(temp))
        {
            return false;
        }
    }
    return true;
}

bool validateUserCred(char* filePath,char* userCred){
    FILE* file = fopen(filePath,"r");
    if (file == NULL)
    {
        printf("Error on opening username file: %s\n",strerror(errno));
        return false;
    }

    char line[MAX_LEN];
    while(fgets(line, MAX_LEN, file) != NULL)
    {
        //line[strlen(line)-1]='\0';
        sprintf(line, "%s", strtok(line, "\n"));
        if(!strcmp(line,userCred))
        {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

userDB* parseUserDB(char** filePath, int* userCnt){
    int userCount = countUsers(filePath);
    *userCnt = userCount;
    userDB* userdb = (userDB*)malloc(sizeof(userDB)*userCount);
    if(userdb == NULL)
    {
        printf("error on malloc for userDb array: %s\n",strerror(errno));
        return NULL;
    }
    FILE* file = fopen(*filePath,"r");
    if (file == NULL)
    {
        printf("Error on opening username file: %s\n",strerror(errno));
        return NULL;
    }
    char line[MAX_LEN];
    const char delim[2] = "\t";

    int i=0;
    while(fgets(line, MAX_LEN, file) != NULL) //fill in userDb array
    {
        if (line[strlen(line)-1]=='\n')
        {
            line[strlen(line)-1]='\0';
        }

        strcpy(userdb[i].name, strtok(line, delim));
        strcpy(userdb[i].password, strtok(NULL, delim));

        for (int j=0 ; j<MAXMAILS ; j++)
        {
            userdb[i].emails[j].deleted = true;
        }

        userdb[i].numOfEmails=0;
        i++;
    }
    fclose(file);
    return userdb;
}

int countUsers(char** filePath){
    FILE* file = fopen(*filePath,"r");
    if (file == NULL)
    {
        printf("Error on opening username file: %s\n",strerror(errno));
        return -1;
    }

    int count=0;
    char line[MAX_LEN];
    while(fgets(line, MAX_LEN, file) != NULL)
    {
        count++;
    }
    fclose(file);
    return count;
}

void printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb){
    char emailLine[MAX_SUBJECT+MAX_USERNAME+10];
    char emailNum[10];
    char emailUsername[MAX_USERNAME];
    char emailSubject[MAX_SUBJECT];
    email* userEmails = userdb[userIndexInDB].emails;

    char numOfEmails[5];
    sprintf(numOfEmails,"%d",userdb[userIndexInDB].numOfEmails);
    numOfEmails[5] = '\0';

    int check = sendFullMsg(clientSocket,numOfEmails);
    if (check <0)
    {
        printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
        close(clientSocket);
        return;
    }

    if (userdb[userIndexInDB].numOfEmails)
    {
      for (int i=0 ; i<MAXMAILS; i++)
        {
            if(!userEmails[i].deleted)
            {
            sprintf(emailNum,"%d",i+1);
            strcpy(emailUsername,userEmails[i].from);
            strcpy(emailSubject,userEmails[i].subject);
            sprintf(emailLine,"%s\t%s\t'%s'\n",emailNum, emailUsername, emailSubject);
            check = sendFullMsg(clientSocket,emailLine);
                if (check <0)
                {
                printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
                close(clientSocket);
                return;
                }
            }
        }
    }
}

void printEmailById(int emailId, int userIndexInDB, int clientSocket, userDB* userdb){

    char emailLine[MAX_CONTENT+MAX_USERNAME*TOTAL_TO+MAX_SUBJECT+MAX_USERNAME];

    char emailUsername[MAX_USERNAME];
    char emailSubject[MAX_SUBJECT];
    char emailTo[MAX_USERNAME*TOTAL_TO];
    char emailContent[MAX_CONTENT];

    email* userEmails = userdb[userIndexInDB].emails;

    //The emailId is minus 1 because we use the place in the array as the email ID
    sprintf(emailUsername,"From: %s\n",userEmails[emailId-1].from);
    sprintf(emailTo,"To: %s\n",userEmails[emailId-1].to);
    sprintf(emailSubject,"Subject: %s\n",userEmails[emailId-1].subject);
    sprintf(emailContent,"Content: %s\n",userEmails[emailId-1].content);

    sprintf(emailLine,"%s%s%s%s",emailUsername,emailTo,emailSubject,emailContent);
    int check = sendFullMsg(clientSocket,emailLine);
    if (check <0)
    {
        printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
        close(clientSocket);
        return;
    }
}

void deleteEmailById(int emailId, int userIndexInDB, userDB* userdb){
    //deleting by setting the email "deleted" value to TRUE
    if(!(userdb[userIndexInDB].emails[emailId-1].deleted)){
        userdb[userIndexInDB].emails[emailId-1].deleted = true;
        userdb[userIndexInDB].numOfEmails--;
    }

}

int insertEmailToDB(char* username, int userCount, int clientSocket, userDB* userdb,char* composeData){

    char* emailTo;
    char* emailSubject; 
    char* emailContent;
    char* recipient;
    int recipientsCount=0;
    const char delim[2] = ",";


    char** usersEmailIsSentTo = (char**)malloc(sizeof(char*)*TOTAL_TO);
    int i;
    for(i=0;i<TOTAL_TO;i++){
        usersEmailIsSentTo[i] = (char*)malloc(sizeof(char)*MAX_USERNAME);
    }

    email* incomingEmail = (email*)malloc(sizeof(email));
    incomingEmail->deleted = false;

    emailTo = strtok(composeData,"\n");
    emailSubject = strtok(NULL,"\n");
    emailContent = strtok(NULL,"\n");


    if(strstr(emailTo, ",") != NULL)
    {
        //getting here means there is more than one recipient
        recipient = strtok(emailTo, delim);
        //Getting all the users the email was sent to, and inserting them into an array
        while(recipient != NULL)
        {
        strcpy(usersEmailIsSentTo[recipientsCount],recipient);
        recipientsCount++;
        recipient = strtok(NULL, delim);
        }
    }
    else
    {
        //getting here means there is only one recipient
        recipient = emailTo;
        strcpy(usersEmailIsSentTo[recipientsCount],recipient);
        recipientsCount=1;
    }

    strcpy(incomingEmail->to,emailTo);
    strcpy(incomingEmail->subject,emailSubject);
    strcpy(incomingEmail->content,emailContent);
    strcpy(incomingEmail->from,username);

    for (int i=0 ; i<userCount; i++)
    {
        //Go over all the users the email was sent to
        for (int j=0; j<recipientsCount ; j++)
        {
            //Find a match between the arrays
            if (!strcmp(userdb[i].name,usersEmailIsSentTo[j]))
            {
                //Insert the email to the emails array of all the users it was sent to
                strcpy(userdb[i].emails[userdb[i].numOfEmails].to,incomingEmail->to);
                strcpy(userdb[i].emails[userdb[i].numOfEmails].subject,incomingEmail->subject);
                strcpy(userdb[i].emails[userdb[i].numOfEmails].from,incomingEmail->from);
                strcpy(userdb[i].emails[userdb[i].numOfEmails].content,incomingEmail->content);
                userdb[i].emails[userdb[i].numOfEmails].deleted = false;
                userdb[i].numOfEmails = (userdb[i].numOfEmails)+1;
                break;
            }
        }
    }

    //free resources
    for(i=0;i<TOTAL_TO;i++){
        free(usersEmailIsSentTo[i]);
    }
    free(usersEmailIsSentTo); 
    sendFullMsg(clientSocket,"Mail sent\n");
    free(incomingEmail);
    return 0;
}

int getUserIndexInDB(userDB* userdb,int userCnt,char* username){
    for (int i=0 ; i<userCnt ; i++)
    {
        if (!strcmp(userdb[i].name,username))
        {
            return i;
        }
    }
    return -1; //if wasnt found returns 1
}

int max(int num1, int num2) {
   int result;

   if (num1 > num2)
      result = num1;
   else
      result = num2;

   return result;
}

int confirmUser(int clientSocket, char* DBPath, char** username){
    char* userCred = (char*)malloc(sizeof(char)*(MAX_USERNAME+MAX_PASSWORD));
    //char* userPass = (char*)malloc(sizeof(char)*MAX_PASSWORD);
    const char delimTab[2] = "\t";
    int check = recvFullMsg(clientSocket,userCred); //receive userName and password
    if (check <0)
    {
        printf("Error receiving client message: %s\nDisconnecting Client.\n",strerror(errno));
        return -1;
    }

    if (!validateUserCred(DBPath,userCred)) //validate that user name and password is correct
    {
        sendFullMsg(clientSocket,"FAIL");
        printf("wrong username & password! Disconnecting client\n");
        return -1;
    }
    else{
        sendFullMsg(clientSocket,"SUCCESS");
        strcpy(*username,strtok(userCred, delimTab));
//      strcpy(userPass,strtok(NULL, delimTab));
    }

    check = sendFullMsg(clientSocket,"Connected to server");

    if (check <0)
    {
        printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
        return -1;
    }
    free(userCred);
    return 0;
}

int showOnlineUsers(clientHandler* fdArray,clientHandler* fdCli, userDB* userdb, int userCount){

    char str[MAX_LEN];
    sprintf(str,"%s","Online users: ");
    for(int i=0;i<userCount;i++){
        if(userdb[i].isActivated){
            sprintf(str,"%s%s, ",str,fdArray[userdb[i].userIndexInClientHandler].username);
        }
    }
    str[strlen(str)-2]='\0';
    if (sendFullMsg(fdCli->fd,str) <0)
    {
        printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
        return -1;
    }

    return 0;
}

int requestForSendingChatMsg(clientHandler* fdCli, clientHandler* fdArray, userDB* userdb, char* actualString, int userCount){
    char temp[MAX_LEN];
    strcpy(temp, actualString);
    char* recipient = strtok(temp, ":");
    char* content = strtok(NULL, ":");
    //content[strlen(content)-1]='\0';

    for(int i=0; i<NUM_OF_CLIENTS; i++){
        if(fdArray[i].isActivated){
            if(!strcmp(recipient, fdArray[i].username)){ //found recipient
                sprintf(actualString,"%s:%s", fdCli->username, content);
                if (sendFullMsg(fdArray[i].fd,actualString) <0) //sending name: content
                {
                    printf("Error sending client message: %s\nDisconnecting recipient Client.\n",strerror(errno));
                    //THIS WILL NOT BE CONSIDERED AN ERROR FOR CURRENT CLIENT, THATS WHY WE CLOSE EVERYTHING HERE AND RETURN 0
                    closeClient(fdArray+i, fdArray, userdb);
                }
                return 0;
            }

        }
    }

    //send email to recipient
    if(insertChatMsgToDB(fdCli->username, recipient, userCount, userdb, actualString) == -1) 
    {
        printf("Error inserting email into DB. problem in server.\n");
        //THIS WILL NOT BE CONSIDERED AN ERROR FOR CURRENT CLIENT, THATS WHY WE CLOSE EVERYTHING HERE AND RETURN 0
    }
    return 0;
}

int insertChatMsgToDB(char* sender, char* recipient, int userCount,userDB* userdb,char* actualString){
    strtok(actualString, ":");
    char* content = strtok(NULL, ":");
    //Go over all the users the email was sent to
    for (int i=0; i<userCount ; i++)
    {
        //Find a match between the arrays
        if (!strcmp(userdb[i].name,recipient))
        {
            //Insert the email to the emails array of all the users it was sent to
            printf("insert msg to mail\n");
            strcpy(userdb[i].emails[userdb[i].numOfEmails].to,recipient);
            strcpy(userdb[i].emails[userdb[i].numOfEmails].subject,"Message received offline");
            strcpy(userdb[i].emails[userdb[i].numOfEmails].from,sender);
            strcpy(userdb[i].emails[userdb[i].numOfEmails].content,(content+1));
            userdb[i].emails[userdb[i].numOfEmails].deleted = false;
            userdb[i].numOfEmails = (userdb[i].numOfEmails)+1;
            break;
        }
    }
    return 0;
}
