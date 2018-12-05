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
#define MAXHOSTNAME 255

typedef struct hostent hostent;
struct sockaddr_in server_addr;
char serverName[MAXHOSTNAME];
int server_fd;
unsigned  short server_port;
int clientLen = sizeof(server_addr);


void serverToClient(int destination, const std::string &msg);
void clientToClient(const std::string &source, std::string destination, std::string msg);

using namespace std;

enum msg_Type {CONNECTION_SUCCESS, CONNECTION_UNSUCCESS, MSG_SENT, MSG_UNSENT, GROUP_CREATED, GROUP_FAILED, UNREGISTERED};
static map<std::string, int> clients;
static map<std::string,std::vector<std::string>> groups ;
fd_set fdSet;

int reset_fdSet()
{


    map<std::string, int>::iterator iter;
    for(auto it = clients.begin(); it != clients.end();++it)
    {

        FD_SET(it->second,&fdSet);
    }

}

/**
 * establish the server so the sever is ready to accept connections from clients
 * */
int starting(unsigned short portNum)
{
    FD_ZERO(&fdSet);
    FD_SET(server_fd,&fdSet);
    FD_SET(STDIN_FILENO,&fdSet);//tirgul p.60

    int socketFd;
    if((socketFd = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        print_error("socket", errno);
    }


    //create the address sruct
    clients["initSocket"] = socketFd;
    memset(&server_addr,0,sizeof(struct sockaddr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portNum);

    //create the socket-->


    if(bind(socketFd,(sockaddr*) &server_addr,sizeof(sockaddr_in))<0)
    {
        //error
        close(socketFd);
        print_error("bind", errno);
        exit(1);
    }

    //start listening to the port
    if(listen(socketFd,10)<0)
    {
        //error
        close(socketFd);
        print_error("listen",errno);
        exit(1);
    }
    return socketFd;

}


void clientToClientVer2(const std::string &source, std::string destination, std::string msg)
{

    std::string msg1 = source + ":" + msg;

    if(clients.find(destination) != clients.end())
    {
        if((send(clients[destination],msg1.c_str(),msg1.size(),0)) != (ssize_t)msg1.size())
        {
            print_error("send",errno);
        }

    }
    else
    {
        print_send(true, false, source, destination, msg);
    }

}

void sendMessageToGroup(const std::string &source, std::string destination, std::string msg)
{
    //only group member can send message to the group-->
    std::vector<std::string> &groupClients = groups.at(destination);////////////////------------
    std::vector<std::string>::iterator it = find(groupClients.begin(),groupClients.end(),source);
    if(it == groupClients.end())
    {
        //the sender is not a part of the group
        print_send(true, false, source, destination, msg);
        serverToClient(clients[source],"ERROR: failed to send message\n");//to be printed by client


    }

    for(auto iter = groupClients.begin(); iter != groupClients.end(); ++iter)
    {
        if((*iter) != source)
        {

            clientToClientVer2(source,(*iter),msg);

        }
    }
    bool success = true;
    std::string msgnew = "sent successfully.\n";
    if((send(clients[source],msgnew.c_str(),msgnew.size(),0)) != (ssize_t)msgnew.size())
    {
        success = false;
    }
    print_send(true, success, source, destination, msg);


}


void clientToClient(const std::string &source, std::string destination, std::string msg)
{

    //check if the group name is unique.
    if(groups.find(destination) != groups.end())
    {

        sendMessageToGroup(source,destination, msg);
        return;
    }
    bool success = true;
    std::string msg1 = source + ":" + msg;

    if(clients.find(destination) != clients.end())
    {
        if((send(clients[destination],msg1.c_str(),msg1.size(),0)) != (ssize_t)msg1.size())
        {
            success = false;
        }
        std::string msgnew = "sent successfully.\n";
        if((send(clients[source],msgnew.c_str(),msgnew.size(),0)) != (ssize_t)msgnew.size())
        {
            success = false;
        }
        print_send(true, success, source, destination, msg);
    }
    else
    {
        print_send(true, false, source, destination, msg);
        std::string msgnew = "ERROR: failed to send.\n";
        if((send(clients[source],msgnew.c_str(),msgnew.size(),0)) != (ssize_t)msgnew.size())
        {
            print_error("send",errno);
            exit(1);
        }
    }

}


void serverToClient(int destination, const std::string &msg)
{
    if ((send(destination, msg.c_str(), msg.size(), 0)) != (ssize_t) msg.size())
    {
        std::cerr<<"ERROR: send"<<errno<<".\n";
        exit(1);
    }
}

void sendConnectionMsg(int socketFD,void *name)
{
    //search if the client in the clients map already
    std::string name1 = (char*) name;
    map<std::string, int>::iterator it;
    for(it = clients.begin(); it != clients.end();++it)
    {
        if(it->first == name1)
        {
            serverToClient(socketFD,"Client name is already in use.\n");
            return;
        }
    }
    map<std::string,std::vector<std::string>> ::iterator it2;
    for(it2 = groups.begin(); it2 != groups.end(); ++it2)
    {
        if(it2->first == name1)
        {

            serverToClient(socketFD,"Client name is already in use.\n");
            return;
        }
    }

    //if we got here -- add the new client to all data structures
    clients[name1] = socketFD;
    print_connection_server(name1);
    serverToClient(socketFD, "Connected successfully.\n");

}
void createGroup(const std::string &clientName,std::string name,std::vector<std::string> &members)
{
    //check if the group name already exist-->
    if(groups.find(name) != groups.end() || clients.find(name) != clients.end())
    {
        print_create_group(true, false,clientName,name);//printed by server
        serverToClient(clients[clientName],"ERROR: failed create_group\n");//to be printed by client
        return;
    }


    //check if all clients exists

    for(std::string client : members)
    {
        if(clients.find(client) == clients.end())
        {
            print_create_group(true, false,clientName,name);//printed by server
            serverToClient(clients[clientName],"ERROR: failed to create group \"" +name+"\".\n");//to be printed by client
            return;
        }

    }
    //if we arrived here - add to data structures
    groups.emplace(name,members);
    print_create_group(true, true,clientName,name);//printed by server
    serverToClient(clients[clientName],"group \"" + name + "\" was created successfully.\n");//to be printed by client




}


void getConnectedClients(std::string clientName)
{
    print_who_server(clientName);
    std::vector<std::string> orderedClients;
    std::string orderedClients2;
    for(auto iter = clients.begin(); iter != clients.end(); ++iter)
    {
        if(iter->first != "initSocket")
        {
            orderedClients.push_back(iter->first );
        }
    }
    std::sort(orderedClients.begin(),orderedClients.end());// sort clients by alphabetical order using strcasemap/*todo*/

    for( auto &client : orderedClients)
    {

        orderedClients2.append(client + ",");

    }

    orderedClients2.erase(orderedClients2.size()-1,1);//removing the last comma
    serverToClient(clients[clientName],orderedClients2);

}
void unregister(std::string clientName)
{
    //find the client in all groups it members and erase it from the relevant group
    for(auto &group :  groups)
    {
        for(std::string &client : group.second)
        {
            if(client == clientName)
            {
                group.second.erase(std::find(group.second.begin(),group.second.end(),clientName));
            }
        }
    }

    print_exit(true,clientName);
    serverToClient(clients[clientName],"Unregistered successfully.\n");
    //try to close the fd of the client
    if(close(clients[clientName]) == -1)
    {
        print_error("close",errno);
        exit(1);
    }
    FD_CLR(clients[clientName],&fdSet);
    clients.erase(clientName);
}


/**
 * Receive client name and command to execute.
 */
void parseSwitch(std::string clientName,std::string command){
    command_type commandT;
    std::string name;
    std::string content;
    std::vector<std::string> clientsNames;
    parse_command(command, commandT, name, content, clientsNames);
    switch (commandT) {
        case CREATE_GROUP:
            if(find(clientsNames.begin(),clientsNames.end(),clientName) == clientsNames.end())
            {
                clientsNames.push_back(clientName);
            }
            createGroup (clientName, name, clientsNames);
            break;
        case SEND:
            clientToClient(clientName, name, content);
            break;
        case WHO:
            getConnectedClients(clientName);
            break;
        case EXIT:
            unregister(clientName);
            break;
        default:
            // INVALID
            print_invalid_input();
            return;
    }
}




/**
 * accepting the connection from socket, reads the data from client, and
 * adding the client to the map
 * */
int accept_new_connection()
{
    void* clientName[255];
    int clientFd;

    //accept returns new socket which is connected to the caller
    if((clientFd = accept(clients["initSocket"],(struct sockaddr*) &server_addr,(socklen_t *)&clientLen))<0)
    {
        print_error("accept",errno);
        exit(1);
        //error
    }
    if(read(clientFd,clientName,255)<0)
    {
        serverToClient(clientFd,"Failed to connect to server.\n");

    }
    sendConnectionMsg(clientFd, (char *)clientName);

}

void exitFromServer()
{
    //take input from stdin and check if "EXIT" was typed
    char buf[255];
    ssize_t numOfBytes = read(STDIN_FILENO,buf,255);
    if(numOfBytes<0)
    {
        print_error("read",errno);
        exit(1);
    }
    std::string bufString = buf;
    bufString.erase(bufString.rfind('\n'));
    if(bufString.find('\n') != std::string::npos)
    {
        bufString.erase(bufString.rfind('\n'));
    }

    if(bufString != "EXIT")
    {
        return;
    }

    for(auto iter = clients.begin(); iter != clients.end(); ++iter)
    {
        if(iter->first != "initSocket")
        {
            unregister((*iter).first);
        }

    }
    FD_ZERO(&fdSet);
    clients.clear();
    groups.clear();
    print_exit();
    exit(0);

}

void handleRequests(int ready, fd_set &readSet){
    char buffer[255];
    for(auto iter = clients.begin(); iter != clients.end(); ++iter)
    {
        if(FD_ISSET(STDIN_FILENO,&readSet)){
            exitFromServer();
            //exit server

        }
        //check for each client fd if it is ready to be read
        if(FD_ISSET((*iter).second,&readSet))
        {

            ssize_t numBytes = read((*iter).second,buffer,255);
            if(numBytes<0)
            {
                std::cerr<<"ERROR:read"<<errno<<".\n";
                exit(1);
            }
            buffer[numBytes] = '\0';
            std::string bufnew = buffer;
            bufnew.erase(bufnew.rfind('\n'));
            if(bufnew.substr(0,5) == "00000")
            {
                ready--;
                break;

            }
            parseSwitch((*iter).first, bufnew);
            ready--;
            if(ready == 0)
            {
                break;
            }
        }
    }
}

/**
 * waits for new connections,accepts them and start reading data
 * */
int connectToClient()
{

    while(true)
    {
        reset_fdSet();
        fd_set readSet = fdSet;
        int ready;

        if((ready = select(FD_SETSIZE,&readSet,NULL,NULL,NULL))>0)
        {
            //there is an FD that is ready for reading
            if(FD_ISSET(server_fd,&readSet))
            {
                //server FD ready -- the client with socket FD is trying to send data
                //so the server accept the connection and read the data
                accept_new_connection();
                //now erase from FD_SET the old socket of the server_fd' before accepting new connection
                FD_CLR(server_fd,&readSet);
                ready--;

                //check each client from the map clients is ready fot reading, and read

            }
            if(FD_ISSET(STDIN_FILENO,&readSet)){
                exitFromServer();
                //exit server

            }
            else{
                handleRequests(ready,readSet);
            }
        }
        else{
            std::cerr<<"ERROR:select"<<errno<<".\n";
            return -1;
        }
    }
}

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        print_server_usage();
        exit(1);
    }

    server_port = (unsigned short) atoi(argv[1]);
    server_fd = starting(server_port);
    connectToClient();
}
