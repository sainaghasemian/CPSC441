// Reference:
// This was recommended by ta
// https://www.geeksforgeeks.org/udp-server-client-implementation-c/amp/
#include <iostream> 
#include <fstream>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <vector>
#include <map>

using namespace std;

#define EFTP_PORT 8020 
#define MAXLINE 1024 
#define AUTH_REQUEST "AUTH"
#define SEG_DATA 1024

std::vector<string> packet_eftp;
struct packet 
{
    string file = "File.txt";

};
int x=0;
int data_recieved(int socket2, sockaddr_in server_addr)
{
    if(x==1)
    {
        return 0;
    }
    char buffer[MAXLINE];
    int data_bytes;
    memset(buffer, 0, sizeof(buffer));
    socklen_t server_addr_l = sizeof(server_addr);
    data_bytes = 0;
    std::string file;
    //set the data bytes recieved to 0
    // Reference: 5-UDP-Protocol Tutorial Slides From Notes
    //receiving
    if((data_bytes = recvfrom(socket2, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &server_addr_l)) ==-1)
    {
        std::cout << "Error: Could not recieve data segment\n" << std::endl;
        exit(1);
    }
    if(data_bytes != 0)
    {
        std::string str= buffer;
        packet_eftp.push_back(str);
        if(data_bytes < 1024)
        {
            x=1;
        }
    }
    return data_bytes;
    // Need to determine how many bytes were in the packet
}

int main(int argc, char **argv) 
{
    // Referenced code: 
    // https://www.ibm.com/docs/en/zos/2.3.0?topic=programs-c-socket-udp-client
    int client_socket, server_port; 
    struct sockaddr_in server_addr;
    
    char segmentbuffer[SEG_DATA+1];

    string username;
    string password;
    string ip;
    string request;
    //int session_number;

    if (argc != 5) 
    {
        std::cout << "Arguments: " << argv[0] << " <ip> <port> <username:password> <read request (RRQ) or a write request (WRQ)> " << std::endl;
        exit(1);
    }

    // Get the server hostname and port number
    ip = argv[1];
    server_port = atoi(argv[2]);
    username = argv[3];
    request = argv[4];
    

    // Create a socket for the client
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) 
    {
        std::cout << "Error socket could not be created \n" << std::endl;
        exit(1);
    }

    // Check for valid ip address from server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) 
    {
        std::cout << "Invalid IP Address \n" << std::endl;
        exit(1);
    }
    
    // Send the AUTH request to the server
    char auth_request[1024];
    memset(auth_request, 0, sizeof(auth_request));
    sprintf(auth_request, "Authenticate: %s\n", username.c_str());
    printf("%s", auth_request);
    //int n = sendto(client_socket, auth_request, strlen(auth_request), 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
  
    if (sendto(client_socket, auth_request, strlen(auth_request), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) 
    {
        std::cout << "Error: could not send authentication request \n" << std::endl;
        exit(1);
    }
  
    // The server sends back another ACK message to acknowledge the WRQ request has been received successfully
    // ACK is recieved 
    // Wait for the server's response
    char buffer[1024];  
    memset(buffer, 0, sizeof(buffer));
    socklen_t len = sizeof(server_addr);
    //int n = recvfrom(client_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &server_addr, &len);
    if (recvfrom(client_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &server_addr, &len) == -1) 
    {
        std::cout << "Error: could not receive server response/ACK \n" << std::endl;
        exit(1);
    }
    
    // If the format is invalid different loop
    int s = -1;
    if(sscanf(buffer, "%d", &s) != 1)
    {
        std::cout << "Error: ACK is in an invalid/incorrect format \n" << std::endl;
        exit(1);
    }
    
    // extract the session number from the server ACK to use it as proof of 
    // its identity for future communications in the same session
    std::cout << "Session number from server, after completion of authentification: " << s << std::endl;

    //RRQ OR WRQ
    // type of request determintation which is being sent to server
    if (request == "RRQ")
    {
        char buffer_msg[1024];
        //the server needs to set the block number to value 1 
        int block_number = 1;
        memset(buffer_msg, 0, sizeof(buffer_msg));
        // send formatted output to the string
        sprintf(buffer_msg, "%d %s %s", 0x01, "File.txt", "\0");
        printf("\n%s", buffer_msg);
        // Reference: Powerpoint tut
        //sendto(int socketid, const void *sendbuf, int sendlen, int flags, const struct sockaddr *to, int tolen)

        if (sendto(client_socket, buffer_msg, strlen(buffer_msg),0 , (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        {
            printf("Unsuccessful send\n");
        }
        while(1)
        {
            int data_bytes; 
            data_bytes = data_recieved(client_socket, server_addr);
            std::string ack_message = "ACK";
            //from server side same code
            if (data_bytes == 0)
            {
                //std::string ack_message = "ACK";
                block_number++;
                if(sendto(client_socket, ack_message.c_str(), ack_message.size(),0 , (struct sockaddr*)&server_addr, len)==-1)
                {
                    printf("ACK was not sent.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            block_number++;
            if(sendto(client_socket, ack_message.c_str(), ack_message.size(),0 , (struct sockaddr*)&server_addr, len)==-1)
            {
                printf("ACK was not sent.\n");
                exit(EXIT_FAILURE);
            }
        }
        //from server side same code
        int new_variable;
        std::string textFile;
        new_variable=0;
        for (const auto& packet_seg : packet_eftp)
        {
            for (int i =0; i < (int) packet_seg.size()-1; i++)
            {
                if(isalpha(packet_seg[i]) || isdigit(packet_seg[i]) || packet_seg[i] == ' ' || packet_seg[i] == '\n')
                {
                    textFile.push_back(packet_seg[i]);
                }


            }
        new_variable++;
        }
        textFile[textFile.size()] = '\0';
        std::ofstream outfile("File.txt");
        if(outfile.is_open())
        {
            //if file is read open
            outfile << textFile;
            outfile.close();
            std::cout << "\nAppropriate content was written into file." << endl;
        }
        else
        {
            std::cout << "\n File could not be open, or does not exist" << endl;
            close(client_socket);
        }
    }

    if (request == "WRQ")
    {
        std::string file;
        //SAME CODE FROM SERVER
        // Referenced: Copied my server code from assignment 1 to read files
        int segmentation_num = 1;
        int byte_block = 0;
        //int block_number = 1;
        int value_determined;
        //create a struct for timeout
        struct timeval timeout;
        //Window socket 
        fd_set window_socket;
        printf("Enter file name: \n");
        //copy file name
        std::cin >> file;
        char buff_message[1024];
        memset(buff_message, 0, sizeof(buff_message));
        sprintf(buff_message, "%d %s %s", 0x02, file.c_str(), "\0");
        if(sendto(client_socket, buff_message, strlen(buff_message),0 , (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
        {
            printf("Unsuccessful request send\n");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        char ack[1024];
        memset(ack, 0, sizeof(ack));
        socklen_t server_addr_length = sizeof(server_addr);
        //recieve ack from server
        if(recvfrom(client_socket, ack, sizeof(ack), 0, (struct sockaddr*)&server_addr, &server_addr_length) == -1)
        {
            printf("Read error\n");
            close(client_socket);
        }
        else
        {
            //write req rec
            printf("Write request read, ACK was recieved by server.\n");
        }

        //SAME CODE FROM SERVER
        // Referenced: Copied my server code from assignment 1 to read files
        FILE *fp = fopen(file.c_str(), "rb");
        if (fp == NULL)
        {
            printf("Failed to open file, or file does not exist in directory.\n");
            close(client_socket);		
        }

        char buffer[8192];
        
        while((byte_block = fread(buffer, 1, 8192, fp))>0)
        {
            for(int i =0; i< 8192; i+= SEG_DATA)
            {
                memset(segmentbuffer, 0, SEG_DATA+1);
                memcpy(segmentbuffer, buffer+i, 1024);
                if(sendto(client_socket, segmentbuffer, sizeof(segmentbuffer),0 , (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
                {
                    printf("Unsuccessful request send\n");
                    close(client_socket);
                    exit(EXIT_FAILURE);
                }
                segmentation_num++;
            }
            
            value_determined = 0;
            while(true)
            {
                 int next;
                //TIMEOUT
                //tv_sec holds the number of seconds, and tv_usec holds the number of microseconds (1,000,000th second)
                //https://www.oreilly.com/library/view/hands-on-network-programming/9781789349863/8e8ea0c3-cf7f-46c0-bd6f-5c7aa6eaa366.xhtml
                timeout.tv_sec = 5;
                timeout.tv_usec =0;

                //initializes the file descriptor set to contain no file descriptors
                FD_ZERO(&window_socket);

                FD_SET(client_socket, &window_socket);
                next = select(client_socket +1, &window_socket, NULL, NULL, &timeout);
                if (next < 0)
                {
                    //use default error
                    fprintf(stderr, "There is an error with selecting");
                    exit(EXIT_FAILURE);
                }
                else if (next == 0)
                {
                    //It is suggested to have at least 3 retries and wait for 5 seconds before the packet times out
                    if (value_determined < 3)
                    {
                        for(int i =0; i< 8192; i+= SEG_DATA)
                        {
                            memset(segmentbuffer, 0, SEG_DATA+1);
                            memcpy(segmentbuffer, buffer+i, 1024);
                            if(sendto(client_socket, segmentbuffer, strlen(segmentbuffer),0 , (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
                            {
                                printf("Unsuccessful request send\n");
                                close(client_socket);
                                exit(EXIT_FAILURE);
                            }
                            segmentation_num++;
                        }
                        value_determined++;
                    }
                    else
                    {
                        //Same as server
                        printf("Error, timed out when wating for ACKS after 3 retries\n");
                        fclose(fp);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    break;
                }
            }
            //completed ACK's  
            char ack_message_buff[1024];
            memset(ack_message_buff, 0 , sizeof(ack_message_buff));
            if(recvfrom(client_socket, ack_message_buff, sizeof(ack_message_buff),0, (struct sockaddr*)&server_addr, &len)==-1)
            {
                printf("Read error\n");
                
            }     

        }
        //DATA
        if(sendto(client_socket, segmentbuffer, strlen(segmentbuffer),0, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
        {
            printf("Error when sending.\n");
            exit(EXIT_FAILURE);
        }
        fclose(fp);
        
    }



    close(client_socket);
    return 0;
}
