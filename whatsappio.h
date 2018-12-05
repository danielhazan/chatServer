#ifndef _WHATSAPPIO_H
#define _WHATSAPPIO_H

#include <cstring>
#include <string>
#include <vector>

#define WA_MAX_NAME 30
#define WA_MAX_MESSAGE 256
#define WA_MAX_GROUP 50
#define WA_MAX_INPUT ((WA_MAX_NAME+1)*(WA_MAX_GROUP+2))

enum command_type {CREATE_GROUP, SEND, WHO, EXIT, INVALID};

/*
 * Description: Prints to the screen a message when the user terminate the
 * server
*/
void print_exit();

/*
 * Description: Prints to the screen a message when the client established
 * connection to the server, in the client
*/
void print_connection();

/*
 * Description: Prints to the screen a message when the client established
 * connection to the server, in the server
 * client: Name of the sender
*/
void print_connection_server(const std::string& client);

/*
 * Description: Prints to the screen a message when the client tries to
 * use a name which is already in use
*/
void print_dup_connection();

/*
 * Description: Prints to the screen a message when the client fails to
 * establish connection to the server
*/
void print_fail_connection();

/*
 * Description: Prints to the screen the usage message of the server
*/
void print_server_usage();

/*
 * Description: Prints to the screen the usage message of the client
*/
void print_client_usage();

/*
 * Description: Prints to the screen the messages of "create_group" command
 * server: true for server, false for client
 * success: Whether the operation was successful
 * client: Client name
 * group: Group name
*/
void print_create_group(bool server, bool success, const std::string& client, const std::string& group);

/*
 * Description: Prints to the screen the messages of "send" command
 * server: true for server, false for client
 * success: Whether the operation was successful
 * client: Client name
 * name: Name of the client/group destination of the message
 * message: The message
*/
void print_send(bool server, bool success, const std::string& client, const std::string& name, const std::string& message);

/*
 * Description: Prints to the screen the messages recieved by the client
 * client: Name of the sender
 * message: The message
*/
void print_message(const std::string& client, const std::string& message);

/*
 * Description: Prints to the screen the messages of "who" command in the server
 * client: Name of the sender
*/
void print_who_server(const std::string& client);

/*
 * Description: Prints to the screen the messages of "who" command in the client
 * success: Whether the operation was successful
 * clients: a vector containing the names of all clients
*/
void print_who_client(bool success, const std::vector<std::string>& clients);

/*
 * Description: Prints to the screen the messages of "exit" command
 * server: true for server, false for client
 * client: Client name
*/
void print_exit(bool server, const std::string& client);

/*
 * Description: Prints to the screen the messages of invalid command
*/
void print_invalid_input();

/*
 * Description: Prints to the screen the messages of system-call error
*/
void print_error(const std::string& function_name, int error_number);

/*
 * Description: Parse user input from the argument "command". The other arguments
 * are used as output of this function.
 * command: The user input
 * commandT: The command type
 * name: Name of the client/group
 * message: The message
 * clients: a vector containing the names of all clients
*/
void parse_command(const std::string& command, command_type& commandT, std::string& name, std::string& messsage, std::vector<std::string>& clients);

#endif
