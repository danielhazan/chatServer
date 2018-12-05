//
// Created by daniel.hazan on 6/16/18.
//


#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <map>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "whatsappio.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#define MAXHOSTNAME 30
#define MAXCHAR 256
typedef struct hostent hostent;
//typedef struct sockadddr sockaddr;
struct sockaddr_in client_addr;
hostent* client_hostent;
//sockaddr_in* client_addr;
unsigned int clientLen = sizeof(sockaddr_in);

fd_set fdSet;

const char *clientName;
int clientSocket;

std::string serverIP;
unsigned short serverPort;
void starting(char** argv)
{
    if((clientSocket = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        print_error("socket", errno);
        exit(1);
    }

    FD_ZERO(&fdSet);
    FD_SET(clientSocket,&fdSet);
    FD_SET(STDIN_FILENO,&fdSet);


    memset(&client_addr,0,sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(argv[2]);
    client_addr.sin_port = htons(serverPort);



}
void connectServer()
{
    if((connect(clientSocket,(struct sockaddr*)&client_addr, sizeof(client_addr)))<0)
    {
        print_error("connect", errno);
        close(clientSocket);
        exit(1);
    }
    if((send(clientSocket,clientName,strlen(clientName)+1,0)) != (ssize_t) strlen(clientName)+1)
    {
        print_error("send", errno);
        exit(1);
    }


}
void sendToClient(const std::string &buff)
{
    if ((send(clientSocket, buff.c_str(), buff.size(), 0)) != (ssize_t) buff.size())
    {
        print_error("send", errno);
        exit(1);
    }
}
void handleUserCommands()
{
    char buffer[MAXCHAR];
    memset(buffer,'0', sizeof(buffer));
    ssize_t numOfBytes = read(STDIN_FILENO,buffer,MAXCHAR);
    if(numOfBytes<0) {
        print_error("read",errno);
    }
    std::string buff(buffer);
    command_type commandT;
    std::string name;
    std::string content;
    std::vector<std::string> clientsNames;
    buff.erase(buff.rfind('\n'));

    try {

        parse_command(buff, commandT, name, content, clientsNames);
    }catch (...){
        print_invalid_input();
    }


    switch (commandT) {
        case CREATE_GROUP:
            if(clientsNames.size()<1 || (clientsNames.size()==1 && clientsNames.front() == clientName))//before sending to server - check if group members are less than 2
            {
                print_create_group(false, false, name,name);
                return;
            }
            break;
        case SEND:
            if(clientName == name)//not allowing to send message to ourselves
            {
                print_send(false, false, name, name, name);
                return;
            }

            break;
        case WHO:

            break;
        case EXIT:

            break;
        case INVALID:
            print_invalid_input();
            return;

    }sendToClient(buff + "\n");

}
void handleServerMsg()
{
    char buffer[MAXCHAR];
    memset(buffer,'0', sizeof(buffer));
    ssize_t numOfBytes = read(clientSocket,buffer,MAXCHAR);
    if(numOfBytes<0)
    {
        print_error("read", errno);
        exit(1);
    }
    std::string text = buffer;
    if(text.substr(0,5)== "ERROR")
    {
        std::cerr<<text.substr(0,(size_t)numOfBytes)<<std::endl;
        return;;

    } else
    {
        //if it is not an error and not send command from another client
        text = text.substr(0,(size_t)numOfBytes);
        const char* text1 = text.c_str();
        if(strcmp(text1,"Client name is already in use. \n")==0)
        {
            print_dup_connection();
            exit(1);
        }
        if(strcmp(text1,"Failed to connect the server\n")==0)
        {
            print_fail_connection();
            exit(1);
        }
        if(strcmp(text1,"Unregistered successfully.\n")==0)
        {
            print_exit(false, text);
            exit(0);
        }


    }
    //other cases
    std::cout<<text<<std::endl;
}

int main(int argc, char** argv)
{
    if(argc!=4)
    {
        print_client_usage();
        exit(1);
    }
    clientName = argv[1];
    unsigned char buf[sizeof(struct in6_addr)];
    memset(buf,'0', sizeof(buf));
    if(inet_pton(AF_INET,argv[2],buf) == 0)
    {
        //check if the address is valid!!!
        print_fail_connection();
        exit(1);
    }
    serverPort = (unsigned  short) atoi(argv[3]);
    starting(argv);
    connectServer();
    int ready = 0;
    while(true)
    {
        fd_set readSet = fdSet;
        if((ready = select(clientSocket+1,&readSet,NULL,NULL,NULL))<0)
        {
            print_error("select",errno);
            exit(1);
        }
        if(ready>0)
        {
            if(FD_ISSET(STDIN_FILENO,&readSet)){
                handleUserCommands();
            }
            else
            {
                handleServerMsg();
            }
        }
    }
}
