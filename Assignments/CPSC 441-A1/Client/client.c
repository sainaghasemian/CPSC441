#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

//constants to be used by client 
#define SEVERPORTNUM 2200

int main()
{
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(SEVERPORTNUM);
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  int mysocket1;
  mysocket1 = socket(AF_INET, SOCK_STREAM, 0);
  if(mysocket1 == -1){
    printf("socket() call failed");
  }

  //1: Create a TCP connection with the server
  int status;
  status = connect(mysocket1, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
  if(status==-1){
    printf("connect() call failed");
  }

  //2: Get user’s UCID as an input in the client and send it to the server
  int count;
  int sentV;
  char ucid[9];
  printf("Please enter your 8 digit UCID: ");
  //read data entered write into 
  scanf("%s", ucid);
  count = strlen(ucid);
  sentV = send(mysocket1, ucid, count, 0);
  if(sentV == -1)
  {
    printf("send() call failed.");
  }

  //3: Read a string which is the current time sent by the server
  int time_of_day;
  char time_now[30];
  time_of_day = recv(mysocket1, time_now, 100, 0);
  if(time_of_day == -1)
  {
		printf("recv() call failed for time.");
	}
  printf("The current time and date from the server: %s\n", time_now);

  //4: Create the PASSCODE, PASSCODE = seconds part of the current time 
  //+ last four digits of UCID
  int temp;
  char passcode[7];
  passcode[0]= time_now[17];
  passcode[1] = time_now[18];
  passcode[2] = ucid[4];
  passcode[3] = ucid[5];
  passcode[4] = ucid[6];
  passcode[5] = ucid[7];
  passcode[6] = '\0';

  //5: Send the PASSCODE to the server
  temp = send(mysocket1, passcode, 100, 0);
  if(temp == -1)
  {
    printf("password send() call failed.");
  }
  //testing print statement to make sure correct password created
  printf("The passcode that is being sent to the server: %s\n", passcode);

  //6: Read the content of the text file sent by the server and
  //save what it reads from the socket into a text file in the run directory

  FILE *fp = fopen("data.txt", "w");
  if(fp == NULL)
  {
    printf("Failed to create file");
  }
  int number_of_bytes;
  char txt_file_content[90];
  number_of_bytes = recv(mysocket1, txt_file_content, 100, 0);
  //REFERNCE: for fwrite
  //https://www.w3resource.com/c-programming/stdio/c_library_method_fwrite.php
  fwrite(txt_file_content, 1, number_of_bytes-1, fp);
  printf("Number of bytes: %d\nReceived from the Server!\n", number_of_bytes);

  if(number_of_bytes == -1)
  {
		printf("recv() call failed for recieving txt file content.");
	}

  //7: Close the socket and file stream
  fclose(fp);
  close(mysocket1);
}


