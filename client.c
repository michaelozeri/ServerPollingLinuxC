#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#define MAX_LEN (1024)
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0) //from http://pubs.opengroup.org/onlinepubs/009695399/functions/bzero.html
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0) //from http://pubs.opengroup.org/onlinepubs/009695399/functions/bcopy.html
#define DEFAULT_PORT 6423
#define TOTAL_TO 20
#define MAXMAILS 32000
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define MAX_SUBJECT 100
#define MAX_CONTENT 100
#define NUM_OF_CLIENTS 20

/*
 * function makes sure all the message inside buf will be sent to the other side using
 * sendall learned in class. must have recvFullMsg on the other side accepting the message
 * returns: -1 on error. 0 on success.
 */
int sendFullMsg(int sock, char* buf);
/*
 * function makes sure all the message inside buf will be received using
 * recvall learned in class. must have sendFullMsg on the other side sending the message
 * returns: -1 on error. 0 on success.
 */
int recvFullMsg(int sock, char* buf);
/*
 * sub function of sendFullMsg
 * sends the length of the buffer before sending the data itself
 * return -1 on ERROR 0 on success
 */
int sendLenOfMsg(int sock, int len);
/*
 * sub function of recvFullMsg
 * receives the length of the buffer before receiving the data itself
 * return -1 on ERROR 0 on success
 */
int recvLenOfMsg(int sock, int *len);
/*
 *receives all data. learned in class
 * return -1 on ERROR 0 on success
 */
int recvall(int s, char *buf, int len);
/*
 * sends all data in buf. learned in class.
 * return -1 on ERROR 0 on success
 */
int sendall(int s, char *buf, int len);
/*
 * a handler for the case the client wants to COMPOSE a mail to the server.
 * will be called from the main function
 * return -1 on ERROR 0 on success
 */
int composeHandler(int sock, char* all, char* all2, char* all3);
/*
 * a handler for the SHOW_INBOX call from the user
 * will be called from the main
 * return -1 on ERROR 0 on success
 */
int showInboxHandler(int sock, char* all, char* all2, char* all3);
/*
 * a handler for the GET_MAIL * call from the user
 * will be called from the main
 * return -1 on ERROR 0 on success
 */
int getMailHandler(int sock, char* all, char* all2, char* all3);
/*
 * a handler for the DELETE_MAIL * call from the user
 * will be called from the main
 * return -1 on ERROR 0 on success
 */
int deleteMailHandler(int sock, char* all, char* all2, char* all3);
/*
 * a handler for the SHOW_ONLINE_USERS call from the user
 * will be called from the main
 * return -1 on ERROR 0 on success
 */
int showOnlineHandler(int sock, char* all, char* all2, char* all3, char** namesOnline);
/*
 * a handler for the MSG *: * call from the user
 * will be called from the main
 * return -1 on ERROR 0 on success
 */
int chatMsgHandler(int sock, char* all, char* all2, char* all3, char* namesOnline);




int onlyNumbers(char* s)
{
    int i=0;
    int len = strlen(s);
    char temp;
    for(i=0; i<len; i++)
    {
        temp = *(s+i);
        if(!isdigit(temp))
        {
            return 0;
        }
    }
    return 1;
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

void closeHandlers(int sock, char* all, char* all2, char* all3){
    if(close(sock)<0)
    {
        printf("failed closing socket\n");
    }
    free(all);
    free(all2);
    free(all3);

}

int max(int num1, int num2) {

   /* local variable declaration */
   int result;

   if (num1 > num2)
      result = num1;
   else
      result = num2;

   return result;
}

int handleEvents(int sock)
{
    char* all = (char*)malloc(sizeof(char)*MAX_LEN);
    char* all2 = (char*)malloc(sizeof(char)*MAX_LEN);
    char* all3 = (char*)malloc(sizeof(char)*MAX_LEN);
    char* namesOnline = (char*)malloc(sizeof(char)*MAX_LEN);
    int ResOfSelect;
    char* username;
    char* content;
    int maxFd = max(0, sock) + 1;
    //create set and initialize it
    fd_set set;
    FD_ZERO(&set);

    while(1)
    {
        FD_SET(sock, &set);
        FD_SET(0, &set); //insert stdin
        ResOfSelect = select(maxFd, &set, NULL, NULL, NULL); 
        if(ResOfSelect < 0){
            printf("Error in select method: %s\n", strerror(errno));
            exit(-1);
        }

        if(FD_ISSET(0, &set)){
            fgets(all, MAX_LEN, stdin);//get input from user
            all[strlen(all)-1]='\0';
            if(!strcmp(all, "QUIT")){
                break;
            }
            else if((!strcmp(all, "SHOW_INBOX")))
            {
                if(showInboxHandler(sock, all, all2, all3) < 0) return 0;
            }

            else if(strstr(all, "GET_MAIL") != NULL)
            {
              if(getMailHandler(sock, all, all2, all3) < 0) return 0;
            }

            else if(strstr(all, "DELETE_MAIL") != NULL)
            {
               if(deleteMailHandler(sock, all, all2, all3) < 0) return 0;
            }


            else if(!strcmp(all, "COMPOSE"))
            {
                if(composeHandler(sock, all, all2, all3) < 0) return 0;
            }

             else if(!strcmp(all, "SHOW_ONLINE_USERS"))
            {
                if(showOnlineHandler(sock, all, all2, all3, &namesOnline) < 0) return 0;
            }
            else if(strstr(all, "MSG") != NULL)
            {

                if(chatMsgHandler(sock, all, all2, all3, namesOnline) < 0) return 0;
            }
            else
            {
                printf("please insert a valid command: SHOW_INBOX, GET_MAIL, DELETE_MAIL or COMPOSE\n");
            }
        }
        if(FD_ISSET(sock, &set)){ //we received message in chat. user: content
            if(recvFullMsg(sock,all) == -1){
                printf("Error accepting message from server:%s\n",strerror(errno));
                closeHandlers(sock, all, all2, all3);
                return -1;
            }

            printf("New message from %s\n",all);
        }
    }

    if(sendFullMsg(sock,"QUIT") == -1){
        printf("Error in sending data to server:%s\n",strerror(errno));
        return -1;
    }

    closeHandlers(sock, all, all2, all3);
    free(namesOnline);
    return 0;
}


void freeAllMain(char* all,char* userName,char* password,char* all2){
    free(all);
    free(userName);
    free(password);
    free(all2);
}

/*
 * the function makes an initial connection to the server (validating username and password)
 * after connecting the function calls HandleEvents function which loops until QUIT is inserted from the user.
 */

int main(int argc, char *argv[])   //probably needs to get port number and host name
{
//we used https://www.youtube.com/watch?v=V6CohFrRNTo for help in writing
//the wrapping of the methods (defining the sockets and struct's, nothing related to
//the business logic))
    int port = DEFAULT_PORT;
    char hostname[MAX_LEN] = "";
    char* all = (char*)malloc(sizeof(char)*MAX_LEN);
    char* userName = (char*)malloc(sizeof(char)*MAX_USERNAME);

    //char password[MAX_PASSWORD] = { '\0' };
    char* password = (char*)malloc(sizeof(char)*MAX_PASSWORD);
    char* all2 = (char*)malloc(sizeof(char)*MAX_LEN);

    int sock = socket(AF_INET, SOCK_STREAM, 0); //was PF_INET
    if(sock<0)
    {
        printf("ERROR opening socket at main:%s\n",strerror(errno));
        freeAllMain(all, userName, password, all2);
        return 0;
    }
    struct sockaddr_in serv_addr;
    struct hostent *server;
    if(argc > 3)   //
    {
        printf("too much arguments. exiting\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return 0;
    }

    //update hostname
    if((argc == 2) || (argc == 3))
    {
        sprintf(hostname, "%s", *(argv+1));
    }
    else  //no arguments given
    {
        sprintf(hostname, "%s%s", hostname, "localhost");
    }

    //update port
    if(argc == 3)
    {
        if(onlyNumbers(argv[2])){
                port = atoi(argv[2]); //get port number.
                if((port > 65535) || (port < 0)){
                    printf("port is not in correct range. exiting\n");
                    close(sock);
                    freeAllMain(all, userName, password, all2);
                    return 0;
                }
        }
        else
        {
            printf("port is not a number. exiting\n");
            close(sock);
            freeAllMain(all, userName, password, all2);
            return 0;
        }
    }

    server=gethostbyname(hostname);
    if(server == NULL)
    {
        printf("problem with name of host. exiting\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return 0;
    }

    //assigning parameters to socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char*) server->h_addr,(char*) &serv_addr.sin_addr.s_addr,server->h_length);

    if(connect(sock, (struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting to server:%s\n",strerror(errno));
        freeAllMain(all, userName, password, all2);
        return -1;
    }

    //parse user name and password from given text
    char* Rusername;
    char* Rpassword;

    if(recvFullMsg(sock, all) == -1)  //this is for accepting the server's welcome message
    {
        printf("ERROR receiving message from server:%s\n",strerror(errno));
        close(sock);
        freeAllMain(all, userName, password, all2);
        return -1;
    }
    printf("%s\n", all);

    fgets(userName, MAX_USERNAME, stdin); //get user name from user

    char delim[2] = " ";
    strtok(userName,delim);
    Rusername = strtok(NULL,delim); //actual username

    int de = strlen(Rusername);
    Rusername[de-1]='\0';

    fgets(password, MAX_PASSWORD, stdin); //get password from user

    strtok(password,delim);
    Rpassword = strtok(NULL,delim);
    Rpassword[strlen(Rpassword)-1]='\0';

    sprintf(all, "%s\t%s", Rusername, Rpassword);
    sendFullMsg(sock, all);
    recvFullMsg(sock, all);

    if(!strcmp(all, "FAIL")){
        printf("wrong username and password combination. program will now exit\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return -1;
    }

    if(recvFullMsg(sock, all2) == -1){ //should be connected to server message
        printf("ERROR receiving message from server\n");
        freeAllMain(all, userName, password, all2);
        close(sock);

        return 0;
    }
    printf("%s\n",all2);
    handleEvents(sock);
    freeAllMain(all, userName, password, all2);
    return 1;
}


int composeHandler(int sock, char* all, char* all2, char* all3){
    char* ToStr;
    char* SubjectStr;
    char* TextStr;
    char* finalStr = (char*)malloc(sizeof(char)*MAX_LEN);

    fgets(all, MAX_LEN, stdin);
    strtok(all,":");
    ToStr = strtok(NULL,":");
    ToStr[strlen(ToStr)-1]='\0';


    fgets(all2, MAX_LEN, stdin);
    strtok(all2,":");
    SubjectStr = strtok(NULL,":");
    SubjectStr[strlen(SubjectStr)-1]='\0';


    fgets(all3, MAX_LEN, stdin);
    strtok(all3,":");
    TextStr = strtok(NULL,":");
    TextStr[strlen(TextStr)-1]='\0';

    if(sprintf(finalStr,"%s %s\n%s\n%s","COMPOSE",ToStr+1,SubjectStr+1,TextStr+1)<0){
        printf("ERROR, in compose handler sprintf:%s\n",strerror(errno));
        return -1;
    }

    if(sendFullMsg(sock, finalStr) == -1) //send receive
    {
        printf("ERROR sending message to server:%s\n",strerror(errno));
        closeHandlers(sock, all, all2, all3);
        return -1;
    }

    //receiving 'mail sent' messege
    if(recvFullMsg(sock,all3) == -1){
        printf("Error in receiving data from server:%s\n",strerror(errno));
        closeHandlers(sock, all, all2, all3);
        return -1;
       }
    
    printf("%s",all3);
    free(finalStr);
    return 0;
}

int showInboxHandler(int sock, char* all, char* all2, char* all3){
    int numOfEmails = 0;
    if(sendFullMsg(sock, all) == -1)
    {
        printf("Error sending message to server:%s\n",strerror(errno));
        closeHandlers(sock, all, all2, all3);
        return -1;
    }

    if(recvFullMsg(sock,all) == -1){ //number of emails
        printf("Error accepting message from server:%s\n",strerror(errno));
        closeHandlers(sock, all, all2, all3);
        return -1;
    }

    numOfEmails = atoi(all);

    if (numOfEmails){
        for(int i=0; i< numOfEmails; i++)
        {
            if(recvFullMsg(sock, all) == -1)
            {
                printf("Error showing inbox:%s\n",strerror(errno));
                closeHandlers(sock, all, all2, all3);
                return -1;
            }
            printf("%s", all);
        }
    }

    return 0;
}

int getMailHandler(int sock, char* all, char* all2, char* all3){
    int serial = atoi(all+strlen("GET_MAIL "));
    if((serial != 0) && (serial > 0) && (serial < MAXMAILS)){ //vaild serial - send it to server
        if(sendFullMsg(sock, all) == -1)
        {
            printf("Error sending message to server:%s\n",strerror(errno));
            closeHandlers(sock, all, all2, all3);
            return -1;
        }
        if(recvFullMsg(sock, all) == -1){ //checking if server completed operation
            printf("Error getting message from server: %s\n",strerror(errno));
            closeHandlers(sock, all, all2, all3);
            return -1;
        }

        if(strcmp(all, "FAIL")){
                if(recvFullMsg(sock, all) == -1) //receive full message
                {
                    printf("Error getting mail: %s\n",strerror(errno));
                    closeHandlers(sock, all, all2, all3);
                    return -1;
                }
                printf("%s", all); //print the mail itself
        }
    }
    else{
        printf("couldn't get specified mail. make sure you entered correct serial\n");
    }

    return 0;
}

int deleteMailHandler(int sock, char* all, char* all2, char* all3){
    int serial = atoi(all+strlen("DELETE_MAIL "));
    if((serial != 0) && (serial > 0) && (serial < MAXMAILS)){ //vaild serial - send it to server
        if(sendFullMsg(sock, all) == -1)
        {
            printf("Error sending message to server: %s\n",strerror(errno));
            closeHandlers(sock, all, all2, all3);
            return -1;
        }
        if(recvFullMsg(sock, all) == -1){ //checking if received ok
            printf("Error accepting message from server: %s\n",strerror(errno));
            closeHandlers(sock, all, all2, all3);
            return -1;
        }
        if(!strcmp(all, "FAIL")){
            printf("couldn't delete specified mail. make sure you entered correct serial\n");
            printf("please enter a new operation\n");
        }

    }

    else{ //invalid serial
        printf("specified ID of mail is invalid. please try again performing an operation\n");
    }
    //success
    return 0;
}

int showOnlineHandler(int sock, char* all, char* all2, char* all3, char** namesOnline){
    int n;
    char * onlyTheNames;
    if(sendFullMsg(sock, all) == -1)
    {
        printf("Error sending message to server - showOnlineHandler:%s\n",strerror(errno));
        closeHandlers(sock, all, all2, all3);
        return -1;
    }
    if((n = recvFullMsg(sock, all2)) == -1){ //checking if server completed operation
        printf("Error getting message from server- showOnlineHandler:%s\n",strerror(errno));
        closeHandlers(sock, all, all2, all3);
        return -1;
    }

    if(strcmp(all2, "FAIL")){ //if we are ok. print online users.
            printf("%s\n", all2); //print the mail itself
    }
    else{
        printf("failed showing online users\n");
        return -1;
    }

    strtok(all2, ":");
    onlyTheNames = strtok(NULL, ":");
    if(strlen(onlyTheNames) > 0){
        strcpy(*namesOnline, onlyTheNames+1);
    }

    return 0;
}

int chatMsgHandler(int sock, char* all, char* all2, char* all3, char* namesOnline){
    all[strlen(all)] = '\0'; //take down \n

    char temp[MAX_LEN]; //inserted this because strtok changes the string itself
    strcpy(temp,all);
    char* helper = strtok(temp, ":");
    strtok(helper, " ");
    char* nameOfUser = strtok(NULL, " ");
    nameOfUser[strlen(nameOfUser)] = '\0';

    //check if user is in online names
    if(strstr(namesOnline, nameOfUser) != NULL)
    {
        if(sendFullMsg(sock, all) == -1)
        {
            printf("Error sending message to server - showOnlineHandler:%s\n",strerror(errno));
            closeHandlers(sock, all, all2, all3);
            return -1;
        }
    }
    else{
        printf("user isn't in online list. update list via SHOW_ONLINE_USERS command, or COMPOSE him a mail\n");
    }
    return 0;

}

