/*
                                ~ USTOR-64 ~

    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>

    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    PHP Function Reference:
    add_ustor(idfa, expirary_in_seconds);
    check_ustor(idfa); (TRUE = Has IDFA, FALSE = Does not have IDFA)

    Performance; READ O(1) - WRITE O(1) prime number hash map using CRC64.

    This version of USTOR uses the UDP protocol for maximum QPS, a hash map
    using a prime number, a simple bucketing system that is adequate enough
    for the purpose of dealing with hash collisions in the application of
    unique filtering and, as usual, cache efficient pre-allocated memory.

    POSIX Threads are used to thread the Read operations, on a separate
    port with port sharing enabled. (port 7811)

    Write operations are single threaded. (port 7810)

    SHA1 is used in the php code to hash the idfa before it is sent to
    the USTOR daemon, once it arrives it is hashed using the CRC64
    algorithm from Redis. Then it is added to the hashmap.

    By default 433,033,301 (433 Million) impressions can be recorded
    not including collisions, which from studying the code you will find
    collisions are range blocked using two short integers to make the
    whole struct a total size of 8 bytes.

    The current configuration uses ~3.2 GB of memory.

    James William Flecher [github.com/mrbid] ~2018
    public release.
*/
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
////////
/*

//PHP Functions for Write and Check operations

function add_ustor($idfa, $expire_seconds)
{
    if($expire_seconds == 0){$expire_seconds = 86400;}
    if($idfa == ''){$idfa = "u";}
    $fp = stream_socket_client("udp://127.0.0.1:7810", $errno, $errstr, 1);
    if($fp)
    {
        fwrite($fp, "$" . sha1($idfa) . "|" . $expire_seconds);
        fclose($fp);
    }
}

function check_ustor($idfa)
{
    if($idfa == ''){$idfa = "u";}
    $fp = stream_socket_client("udp://127.0.0.1:7811", $errno, $errstr, 1);
    if($fp)
    {
        $r = fwrite($fp, sha1($idfa));
        if($r == FALSE)
        {
            fclose($fp);
            return TRUE;
        }
        //stream_set_timeout($fp, 1);
        $r = fread($fp, 1);
        if($r != FALSE && $r[0] == 'n')
        {
            fclose($fp);
            return FALSE; //The only time we can bid, is if the server 'says-so'
        }
        fclose($fp);
        return TRUE;
    }
    return TRUE;
}

*/
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
////////

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h> //atoi()
#include <time.h> //time()
#include <signal.h> //SIGPIPE
#include <netdb.h> //hostent
#include <sys/sysinfo.h> //CPU cores
#include <pthread.h> //Threading
#include <unistd.h> //Sleep
#include "crc64.h" //crc64

//433033301 = ~3.1 GB
#define MAX_SITES 433033301 //Max number of impressions possible to track (if you change this number, make sure you use a prime number)
#define RECV_BUFF_SIZE 1024

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
////////

//Max lenth of small strings
#define MIN_LEN 256
#define uint unsigned int
#define ulong unsigned long long int

void timestamp()
{
    time_t ltime = time(NULL);
    fprintf(stderr, "\n%s", asctime(localtime(&ltime)));
}

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
////////
///
//
//
//
/*
.
.
.
.
. ~ Unique Capping Code
*/

//Impressions are split into site domain buckets
struct site //8 bytes, no padding.
{
    unsigned short idfa_high;
    unsigned short idfa_low;
    uint expire_epoch;
};

//Our buckets
struct site *sites;

//Logs
ulong blocked=0, allowed=0, rejected=0, collisions=0;

void init_sites()
{
    sites = malloc(MAX_SITES * sizeof(struct site));
    if(sites == NULL)
    {
        perror("Failed to allocate memory");
        exit(0);
    }

    for(int i = 0; i < MAX_SITES; ++i)
    {
        sites[i].idfa_high = 0;
        sites[i].idfa_low = 0;
        sites[i].expire_epoch = 0;
    }
}

//Check against all idfa in memory for a match, if it's not there, add it,
int has_idfa(const uint64_t idfa) //Pub
{
    //Find site index
    const uint site_index = idfa % MAX_SITES;

    //Reset if expire_epoch dictates this site bucket is expired 
    if(time(0) >= sites[site_index].expire_epoch) //Threaded race conditions are not an issue here.
    {
        sites[site_index].idfa_low = 0;
        sites[site_index].idfa_high = 0;
        sites[site_index].expire_epoch = 0;

        //timestamp();
        //fprintf(stderr, "%i: reached it's expirary after %u hours.\n", cid_index, ((time(0) - campaigns[cid_index].sites[site_index].last_epoch) / 60) / 60 );
    }

    //Set the ranges
    unsigned short idfar = (idfa % (sizeof(unsigned short)-1))+1;
    if(idfar >= sites[site_index].idfa_low && idfar <= sites[site_index].idfa_high)
    {
        rejected++;
        return 1; //it's blocked
    }

    //Stats
    allowed++;

    //add_idfa_priv(site_index, idfa); //No idfa? Add it
    return 0; //NO IFDA :(
}

//Set's ifa,
void add_idfa(const uint64_t idfa, const uint expire_seconds) //Pub
{
    //Find site index
    const uint site_index = idfa % MAX_SITES;

    //Reset if expire_epoch dictates this site bucket is expired 
    if(time(0) >= sites[site_index].expire_epoch)
    {
        sites[site_index].idfa_low = 0;
        sites[site_index].idfa_high = 0;
        sites[site_index].expire_epoch = time(0)+expire_seconds;

        //timestamp();
        //fprintf(stderr, "%i: reached it's expirary after %u hours.\n", cid_index, ((time(0) - campaigns[cid_index].sites[site_index].last_epoch) / 60) / 60 );
    }

    //Looks like this is going to be a collision, not a fresh slot
    if(sites[site_index].idfa_low != 0)
        collisions++;

    //Set the ranges
    unsigned short idfar = (idfa % (sizeof(unsigned short)-1))+1;
    if(idfar < sites[site_index].idfa_low || sites[site_index].idfa_low == 0)
    {
        sites[site_index].idfa_low = idfar;
    }
    if(idfar > sites[site_index].idfa_high || sites[site_index].idfa_high == 0)
    {
        sites[site_index].idfa_high = idfar;
    }

    //Increment blocked
    blocked++;

    //if(r==1 && campaigns[cid_index].sites[site_index].idfa_high != campaigns[cid_index].sites[site_index].idfa_low)
        //printf("%i: %i - %i = %i\n", site_index, campaigns[cid_index].sites[site_index].idfa_low, campaigns[cid_index].sites[site_index].idfa_high, campaigns[cid_index].sites[site_index].idfa_high - campaigns[cid_index].sites[site_index].idfa_low);
}

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////
///////////////
////////
///
//
//
//
/* ~ Socket I/O interface to the keystore
*/

void parseMsg(const char* str, char* idfa, char* expire)
{
    char *wh = idfa;
    int i = 0;
    if(str[0] == '$')
        i = 1;
    const int len = strlen(str);
    for(; i < len; i++)
    {
        if(str[i] == '|')
        {
            *wh = 0x00;
            wh = expire;
            continue;
        }

        *wh = str[i];
        wh++;
    }
    *wh = 0x00;
}

//Test thread
/*void *testThread(void *arg)
{
    //Never allow process to end
    ulong err = 0;
    uint reqs = 0;
    time_t st = time(0);
    time_t tt = time(0);
    while(1)
    {
        //Log Requests per Second
        if(st < time(0))
        {
            printf("WRITE: Req/s: %ld, Blocked: %llu, Collisions: %llu, Allowed: %llu, Rejected: %llu, Errors: %llu\n", reqs / (time(0)-tt), blocked, collisions, allowed, rejected, err);

            //Prep next loop time
            tt = time(0);
            reqs = 0;
            st = time(0)+3;//+180;
        }

        char pidfa[RECV_BUFF_SIZE];
        sprintf(pidfa, "%u-%u", rand(), rand());
        const uint64_t idfa = crc64(0, (unsigned char*)pidfa, strlen(pidfa));
        has_idfa(idfa);
        add_idfa(idfa, 172800);
        reqs++;
    }
}*/

//Read thread
void *readThread(void *arg)
{
    //Suppress sigpipe
    signal(SIGPIPE, SIG_IGN);

    //From here on out, nothing will crash or fail, it will restart
    while(1)
    {   
        //Vars
        struct sockaddr_in server, client;
        unsigned int slen = sizeof(client);

        //Create socket
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(s == -1)
        {
            sleep(3);
            continue;
        }

        //Allow the port to be reused
        int reuse = 1; //mpromonet [stack overflow]
        if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
            perror("setsockopt(SO_REUSEADDR) failed");
        if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
            perror("setsockopt(SO_REUSEPORT) failed");

        //Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(7811);
        
        //Bind port to socket
        while(bind(s, (struct sockaddr*)&server, sizeof(server)) < 0)
            sleep(3);

        //Never allow process to end
        ulong err = 0;
        uint reqs = 0;
        time_t st = time(0);
        time_t tt = time(0);
        int read_size;
        char client_buff[RECV_BUFF_SIZE];
        while(1)
        {
            //Client Command
            memset(client_buff, '\0', sizeof(client_buff));
            read_size = recvfrom(s, client_buff, RECV_BUFF_SIZE-1, 0, (struct sockaddr *)&client, &slen);
            //printf("%s\n", client_buff);

            //Log Requests per Second
            if(st < time(0))
            {
                printf("READ: Req/s: %ld, Errors: %llu\n", reqs / (time(0)-tt), err);

                //Prep next loop time
                tt = time(0);
                reqs = 0;
                st = time(0)+180;
            }

            //Nothing from the client?
            if(read_size <= 0)
            {
                err++; //Transfer error
                continue;
            }
            reqs++;

            //Parse the message
            const uint64_t idfa = crc64(0, (unsigned char*)client_buff, strlen(client_buff));

            if(has_idfa(idfa) == 0)
            {
                if(sendto(s, "n", 1, 0, (struct sockaddr*)&client, slen) < 0)
                    err++; //Connection Error
            }
            else
            {
                if(sendto(s, "y", 1, 0, (struct sockaddr*)&client, slen) < 0)
                    err++; //Connection Error
            }

            //
        }
    }

    return 0;
}

//Main process for writing (you could also read, but there's no point unless your going single threaded)
int main(int argc , char *argv[])
{
    //Init memory for unique filtering
    init_sites();

    //Prevent process termination from broken pipe signal
    signal(SIGPIPE, SIG_IGN);

    //Launch Info
    timestamp();
    printf("USTOR - James William Fletcher\n");
    printf("https://github.com/mrbid\n");
    printf("US-64\n");

    //Launch read threads
    printf("%i CPU Cores detected..\n", get_nprocs());
    for(int i = 0; i < get_nprocs(); i++)
    {
        pthread_t tid;
        if(pthread_create(&tid, NULL, readThread, NULL) != 0)
        {
            perror("Failed to launch Read Thread.");
            return 0;
        }
    }

    //Launch test thread
    /*pthread_t tid;
    if(pthread_create(&tid, NULL, testThread, NULL) != 0)
    {
        perror("Failed to launch TEST Thread.");
        return 0;
    }*/
     
    //From here on out, nothing will crash or fail, it will restart
    while(1)
    {
        //Vars
        struct sockaddr_in server, client;
        unsigned int slen = sizeof(client);

        //Create socket
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(s == -1)
        {
            perror("Failed to create write socket ...");
            sleep(3);
            continue;
        }
        puts("Socket created...");

        //Allow the port to be reused
        int reuse = 1; //mpromonet [stack overflow]
        if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
            perror("setsockopt(SO_REUSEADDR) failed");
        if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
            perror("setsockopt(SO_REUSEPORT) failed");

        //Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(7810); // listen on port
        
        //Bind port to socket
        time_t tt = time(0); 
        while(bind(s, (struct sockaddr*)&server, sizeof(server)) < 0)
        {
            //print the error message
            perror("bind error");
            printf("time taken: %lds\n", time(0)-tt);
            sleep(3);
        }
        printf("Bind took: %.2f minutes ...\n", (float)(time(0)-tt) / 60.0f);
        puts("Waiting for connections...");

        //Never allow process to end
        ulong err = 0;
        uint reqs = 0;
        time_t st = time(0);
        tt = time(0);
        int read_size;
        char client_buff[RECV_BUFF_SIZE];
        char pidfa[RECV_BUFF_SIZE];
        char pexpire[RECV_BUFF_SIZE];
        while(1)
        {
            //Client Command
            memset(client_buff, '\0', sizeof(client_buff));
            read_size = recvfrom(s, client_buff, RECV_BUFF_SIZE-1, 0, (struct sockaddr *)&client, &slen);
            //printf("%s\n", client_buff);

            //Nothing from the client?
            if(read_size <= 0)
            {
                err++; //Transfer error
                continue;
            }
            reqs++;

            //Log Requests per Second
            if(st < time(0))
            {
                printf("WRITE: Req/s: %ld, Blocked: %llu, Collisions: %llu, Allowed: %llu, Rejected: %llu, Errors: %llu\n", reqs / (time(0)-tt), blocked, collisions, allowed, rejected, err);

                //Prep next loop time
                tt = time(0);
                reqs = 0;
                st = time(0)+180;
            }

            //Parse the message
            pidfa[0] = 0x00;
            pexpire[0] = 0x00;
            parseMsg(client_buff, pidfa, pexpire);
            if(pidfa[0] == 0x00)
            {
                err++; //Transfer error
                continue;
            }
            uint expire_seconds = atoi(pexpire);
            if(expire_seconds > 172800) //Max expire 48 hours
                expire_seconds = 172800;
            const uint64_t idfa = crc64(0, (unsigned char*)pidfa, strlen(pidfa));

            //Check if we have this idfa
            if(client_buff[0] != '$')
            {
                if(has_idfa(idfa) == 0)
                {
                    if(sendto(s, "n", 1, 0, (struct sockaddr*)&client, slen) < 0)
                        err++; //Connection Error
                }
                else
                {
                    if(sendto(s, "y", 1, 0, (struct sockaddr*)&client, slen) < 0)
                        err++; //Connection Error
                }
            }
            else
            {
                if(pexpire == 0x00)
                {
                    err++; //Transfer error
                    continue;
                }

                add_idfa(idfa, expire_seconds);
            }

            //
        }
    }
    
    //Daemon
    return 0;
}


