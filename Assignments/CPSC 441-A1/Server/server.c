#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define SEVERPORTNUM 2200

int main()
{
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(SEVERPORTNUM);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	int mysocket1;
	mysocket1 = socket(AF_INET, SOCK_STREAM, 0);
	if(mysocket1 == -1){
		printf("socket() call failed");
	}
	
	int status;
	status = bind(mysocket1, (struct sockaddr *)
	&address, sizeof(struct sockaddr_in));
	if(status==-1){
		printf("bind() call failed");
	}

	//1: while TRUE do
	while(1)
	{
		//2: Listen for incoming requests (from clients using TCP) over a specific port
		status = listen(mysocket1,5);
		if(status==-1){
			printf("listen() call failed");
		}
		//3: Accept a new connection
		int mysocket2;
		mysocket2 = accept(mysocket1, NULL, NULL);
		if(mysocket2 == -1){
			printf("accept() call failed");
		}
		
		//4: Receive user’s UCID from the client
		int count; 
		char ucid[9];
		count = recv(mysocket2, ucid, 8, 0);
		if(count == -1){
			printf("recv() call failed.");
		}
		//just a test print statement to make sure correct ucid was entered
		printf("The received UCID from the client: %s\n", ucid);

		// REFRENCE to send current day and time to the connected client: 
		// https://www.tutorialspoint.com/c_standard_library/c_function_time.htm
		// https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
		// to get time of day

		//5: Send current day and time to the connected client
		time_t timeValue = time(NULL);
		struct tm *localT = localtime(&timeValue);
		char time_now[30];
		strftime(time_now, sizeof(time_now), "%c", localT);

		send(mysocket2, time_now, strlen(time_now), 0);

		//6: Create a PASSCODE.
		//PASSCODE = seconds part of that date time + last four digits of UCID
		char server_passcode[7];
		server_passcode[0]= time_now[17];
		server_passcode[1] = time_now[18];
		server_passcode[2] = ucid[4];
		server_passcode[3] = ucid[5];
		server_passcode[4] = ucid[6];
		server_passcode[5] = ucid[7];
		server_passcode[6] = '\0';

		//testing print statement to make sure correct password created
		//printf("%s", passcode);

		//7: Recieve an integer from the client
		//It should be the same passcode but constructed and sent by the client
		//Recieving password from client
		char recv_pass[7];
		count = recv(mysocket2, recv_pass, 6, 0);
		if(count == -1)
		{
			printf("recv() call for recieving password from client failed.");
		}
		//test print statement to make sure correct password was recieved
		printf("The password recieved from the client: %s\n", recv_pass);
		//if (server_passcode == recv_pass)
		if (strcmp(server_passcode, recv_pass) == 0)
		{
			//printf("pass equal");
			//8: Open the file \data.txt" from the run directory and send its contents to client
			// Open file and send contents to client
			FILE *fp = fopen("data.txt", "r");
			char whats_inside[90];
			if (fp == NULL)
			{
				printf("Failed to open file, or file does not exist in directory.\n");
				close(mysocket2);
				close(mysocket1);
			}
			else
			{
				//printf("File found. \n");
				//REFERENCE: 
				//https://www.geeksforgeeks.org/c-program-to-read-contents-of-whole-file/
				//on how to read file
				int i = 0;
				while (!feof(fp)) 
				{
					whats_inside[i] = fgetc(fp);
					i++;
				}
		
				int count = strlen(whats_inside);
				send(mysocket2, whats_inside, count, 0);
				
			}
			fclose(fp);
		}
		else
		{
			printf("Password constructed by server and client do not match. \n");
		}
		close(mysocket2);
	}
	//9:Close the client socket
	//close(mysocket2);
	close(mysocket1);
}
