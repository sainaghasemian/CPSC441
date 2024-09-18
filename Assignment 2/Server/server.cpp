// Reference:
// This was recommended by ta
// https://www.geeksforgeeks.org/udp-server-client-implementation-c/amp/

// Server side implementation of UDP client-server model 
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
#define SEG_DATA 1024

vector<string> packet_eftp;
//generated session number
int session_number;

// Function to generate a random session number
/*int generate_session_number() 
{
    // The server generate a random session number for the client
    int session_number;
    srand(time(NULL));
    session_number = rand() % 65535 + 1;
    return session_number;
}
*/
int x=0;

std::map<string, string> userpass;
//If the provided credentials (username and password) match with the records
// Reference:
// https://stackoverflow.com/questions/16037683/way-to-recognize-client-at-server-side
// Used this site for support on how to check if credentials were correct
bool authenticate_user_and_pass(const string& username, const string& password)
{
    userpass.insert({"sa", "12"});
    auto it = userpass.find(username);
    if (it != userpass.end() && it->second == password)
    {
        // dtermine if user credentials match
        return true;
    }
    return false;


}

// Referenced from client side
int data_recieved(int socket4, sockaddr_in* client_addr)
{
    x++;
    char buffer[MAXLINE];
    int data_bytes;
    memset(buffer, 0, sizeof(buffer));
    socklen_t client_addr_l = 16;
    data_bytes = 0;
    //set the data bytes recieved to 0
    // Reference: 5-UDP-Protocol Tutorial Slides From Notes
    //receiving
    if((data_bytes = recvfrom(socket4, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_addr_l)) ==-1)
    {
        std::cout << "Error: Could not recieve data \n" << std::endl;
        exit(1);
    }
    if(strlen(buffer) != 0)
    {
        std::string str= buffer;
        packet_eftp.push_back(str);
        
    }
    return strlen(buffer);
    // Need to determine how many bytes were in the packet
}


void data_transmission_phase(int socket, const sockaddr_in* client_addr, int session_number, int number_determined, int segment, const char* segment_buffer)
{
    char buffer[1024];
    int ran = 0;
    strcpy(buffer+ran, segment_buffer);
    ran += strlen(segment_buffer)+1;

    //end of message
    buffer[ran] = '\0';
    //send to client 
    // Refrence: the tutorial udp slides for sending
    if(sendto(socket, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, 16) ==-1)
    {
        //printf("Unsuccessful send\n");
        exit(EXIT_FAILURE);
    }


}


// If it is a read request (RRQ), the server will start immediately transmitting 
// the first segment of the file that the client has requested using a DATA message.
void read_request(int socket, struct sockaddr_in* client_addr, int session, const string file) 
{
    // the server will start immediately transmitting the first segment of the file 
    // that the client has requested using a DATA message.
    
    char buffer[8192];
    char segment[SEG_DATA+1];
    int segmentation_num = 1;
    int byte_block = 1;
    int bytes_to_read;
    socklen_t client_addr_length = sizeof(*client_addr);
    //create a struct for timeout
    struct timeval timeout;
    //Window socket 
    fd_set window_socket;
    // Referenced: Copied my server code from assignment 1 to read files
    FILE *fp = fopen("File.txt", "rb");
    if (fp == NULL)
	{
		printf("Failed to open file, or file does not exist in directory.\n");
		close(socket);		
	}
    
    //printf("File found. \n");
    //REFERENCE: 
    //https://www.geeksforgeeks.org/c-program-to-read-contents-of-whole-file/
    //on how to read file
    while((bytes_to_read = fread(buffer, 1, 8192, fp))>0)
    {
        int value_determined;
        for(int i =0; i< 8192; i+= SEG_DATA)
        {
            memset(segment, 0, SEG_DATA+1);
            memcpy(segment, buffer+i, 1024);
            segment[strlen(segment)] = '\0';
            //FIX THIS PART
            data_transmission_phase(socket, client_addr, session_number, byte_block, segmentation_num, segment);
            segmentation_num++;
        }
        value_determined = 0;
        while(1)
        {
            int next;
            //TIMEOUT
            //tv_sec holds the number of seconds, and tv_usec holds the number of microseconds (1,000,000th second)
            //https://www.oreilly.com/library/view/hands-on-network-programming/9781789349863/8e8ea0c3-cf7f-46c0-bd6f-5c7aa6eaa366.xhtml
            timeout.tv_sec = 5;
            timeout.tv_usec =0;

            //initializes the file descriptor set to contain no file descriptors
            FD_ZERO(&window_socket);

            FD_SET(socket, &window_socket);
            next = select(socket +1, &window_socket, NULL, NULL, &timeout);

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
                        memset(segment, 0, SEG_DATA+1);
                        memcpy(segment, buffer+i, 1024);
                        segment[strlen(segment)] = '\0';
                        //FIX THIS PART
                        data_transmission_phase(socket, client_addr, session_number, byte_block, segmentation_num-8, segment);
                        segmentation_num++;
                    }
                    value_determined++;
                }
                else
                {
                    printf("Error, timed out when wating for ACKS after 3 retries\n");
                    fclose(fp);
                }
            }
            else
            {
                break;
            }
        }
        char ack_message_buff[1024];
        memset(ack_message_buff, 0 , sizeof(ack_message_buff));
        if(recvfrom(socket, ack_message_buff, sizeof(ack_message_buff),0, (struct sockaddr*)&client_addr, &client_addr_length)==-1)
        {
            printf("Read error\n");
           
        }
    }
    
    data_transmission_phase(socket, client_addr, session_number, byte_block, 1, "");
    fclose(fp); 
}


//if it is a write request (WRQ), the server sends back another ACK message to acknowledge the WRQ request has been received successfully. 
//In this case, the server needs to set the block number to value 1 but leave the segment number to 0, this is needed to differentiate the
//current ACK from the last ACK message.
void write_request(int socket, sockaddr_in* client_addr, const string file)
{
    //int block_number = 1;
    int segmentation_number, data_bytes, variable;
    segmentation_number = 1; // did not assign to 0 bc original 1
    variable = 0;
    std::string textFile;

    // Reference:
    // https://www.youtube.com/watch?v=2hNdkYInj4g
    while(1)
    {
        data_bytes = data_recieved(socket, client_addr);
        std::string ack_message = "ACK";
        if (data_bytes == 0)
        {
            //std::string ack_message = "ACK";
            
            if(sendto(socket, ack_message.c_str(), ack_message.size(),0 , (struct sockaddr*)&client_addr, sizeof(client_addr)*2)==-1)
            {
                printf("ACK was not sent.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        if(sendto(socket, ack_message.c_str(), ack_message.size(),0 , (struct sockaddr*)&client_addr, sizeof(client_addr)*2)==-1)
        {
            printf("ACK was not sent.\n");
            exit(EXIT_FAILURE);
        }
        segmentation_number++;
            

    }

    for (const auto& packet_seg : packet_eftp)
    {
        for (int i =0; i < (int) packet_seg.size()-1; i++)
        {
            if(isalpha(packet_seg[i]) || isdigit(packet_seg[i]) || packet_seg[i] == ' ' || packet_seg[i] == '\n')
            {
                textFile.push_back(packet_seg[i]);
            }


        }
        variable++;
    }
    std::ofstream outfile(file.c_str());
    if(outfile.is_open())
    {
        //if file is read open
        outfile << textFile;
        outfile.close();
        std::cout << "\n Appropriate content was written into file." << endl;
    }
    else
    {
        std::cout << "\n File could not be open, or does not exist" << endl;
    }

}

int main() 
{ 
    int socket1; 
    srand(time(NULL));
    socklen_t length_for_addr;
    struct sockaddr_in server_addr;
    struct sockaddr_in* client_addr;
    char client_req[1024];
    bool authentication_phase;
    string username;
    string password;

    // Creating socket file descriptor 
    if((socket1 = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Binding socket to port/Filling server information 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((uint16_t)EFTP_PORT); // EFTP server listening on port 5000
    if (bind(socket1, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("Socket bind failed, could not bond to port");
        exit(EXIT_FAILURE);
    }

    // Connection is made, begin authentication, sending, receiving data
    std::cout << "The server is now listening on port [In authentication phase]" << std::endl;
    // Current authentication phase
    authentication_phase = false;
    length_for_addr = sizeof(*client_addr);
    memset(&client_addr, 0, length_for_addr);
    char buffer_2[MAXLINE];
    while(!authentication_phase)
    {
        int var;

        // Recieve AUTH RRQ from the client side 
        var = recvfrom(socket1, buffer_2, 1024, 0, (struct sockaddr *)&client_addr, &length_for_addr);
        
        // Check if the received message is an AUTH request
        //CHECKKKKK
        if (var == -1) 
        {
            std::cerr << "Faliure recieving AUTHENTICATION from client.\n";
            exit(EXIT_FAILURE);
        }

        // Parse the clients's response
        // response based on user:pass
        string response(buffer_2, var);
        size_t message_rep = response.find(":");
        if (message_rep == string::npos) 
        {
            std::cout << "Authentication failed" << std::endl;
            continue;
            
        } 

        // Determine username and password sent from client 
        username = response.substr(5,2);
        password = response.substr(message_rep+1);


        //Here you need to be able to check if the username and pass sent from client match the ones in server
        //If the provided credentials (username and password) match with the records, 
        //the server generates a random positive integer

        bool authenticate=true;
        authenticate = authenticate_user_and_pass(username, password);
        if(authenticate)
        {
            //if not able to authenticate from records
            std::string error = "Not able to authenticate client";
            sendto(socket1, error.c_str(), error.size(), 0, (struct sockaddr*)&client_addr, length_for_addr);
            continue;
        }
        
        
        // Send an authentication succesful message with the session number to the client side
        
        session_number = rand() % 65535 + 1;
        //the server generates a random positive integer
        //ACK
        std::string ack_new_message;
        ack_new_message = std::to_string(session_number) + ":ACK";
        //send ACK message and session num to client
        
        sendto(socket1, ack_new_message.c_str(), ack_new_message.size(), 0 ,(struct sockaddr*)&client_addr, length_for_addr);
        // Print out sessions number
        std::cout << "Authentication successful for client at session number: " <<session_number << endl;

        //authentication verified
        authentication_phase = true;    
    }
    
    socklen_t client_bytes_rec_len = sizeof(struct sockaddr_in);
    size_t data_bytes = recvfrom(socket1, &client_req, strlen(client_req), 0, (struct sockaddr*)&client_addr, &client_bytes_rec_len);
    //recieve data bytess from client
    if(data_bytes < 0)
    {
        printf("Read error\n");
        close(socket1);
    }

    //RRQ OR WRQ
    // type of request determintation from client side
    
    if (client_req[0] == 49)
    {
        //RRQ
        printf("Read request recieved (RRQ) for file: %s\n", "File.txt");
        read_request(socket1, client_addr, session_number, "File.txt");


    }
    if (client_req[0] == 50)
    {
        //WRQ
        printf("Write request recieved (WRQ)\n");
        //SAME AS FROM WRITE REQ
        std::string ack_new_message = std::to_string(session_number) + ":ACK";
        sendto(socket1, ack_new_message.c_str(), ack_new_message.size(), 0 ,(struct sockaddr*)&client_addr, length_for_addr);
        write_request(socket1, (struct sockaddr_in*) client_addr, "File.txt");
    }
 
    close(socket1);
    return 0;
}