// James William Flecher [github.com/mrbid] ~2018
// Not for public release.
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
//////// 

#include <stdio.h>
#include <string.h>
#include <stdlib.h> //atoi()
#include <time.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h> //SIGPIPE
#include <pthread.h> //Threading

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
////////

void csend(const char* ip, const uint16_t port, const char* send)
{
    struct sockaddr_in server;

    int s;
    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return;

	memset((char*)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	if(inet_aton(ip, &server.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		return;
	}

    //Write and close
    sendto(s, send, strlen(send), 0, (struct sockaddr*)&server, sizeof(server));
    close(s);
}

void *sendThread(void *arg)
{
    signal(SIGPIPE, SIG_IGN);

    while(1)
    {
        char rr[256];
        sprintf(rr, "%i", rand());
        csend("127.0.0.1", 7811, rr);

        char rw[256];
        sprintf(rw, "$%i|86400", rand());
        csend("127.0.0.1", 7810, rw);

        //usleep(1000);
    }
}

int main(int argc , char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    //Launch some threads
    pthread_t tid;
    for(int i = 0; i < 4; i++)
    {
        if(pthread_create(&tid, NULL, sendThread, NULL) != 0)
        {
            perror("Failed to launch");
            break;
        }
    }

    //loop
    while(1)
    {
        char rr[256];
        sprintf(rr, "%i", rand());
        csend("127.0.0.1", 7811, rr);

        char rw[256];
        sprintf(rw, "$%i|86400", rand());
        csend("127.0.0.1", 7810, rw);

        //usleep(1000);
    }
}


