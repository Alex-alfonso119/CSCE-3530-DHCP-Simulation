/*
    Alexander Alfonso (aja0167) - CSCE3530: PROGRAM 5
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define SERVER "129.120.151.94"
#define MAXSIZE 16

#pragma pack(1)     //  turn on pragma pack to send struct
struct dhcp_pkt
{
	unsigned int siaddr;			//	server iP addr
	unsigned int yiaddr;			//	your IP addr
	unsigned int t_id;				//	transaction ID
	unsigned short int lifetime;	//	lease time of IP addr
};
#pragma pack(0)     //  turn off pragma pack

void printPkt(struct dhcp_pkt packet, unsigned int gate[4]);
void parseGate(char *addr, unsigned int gate[4]);
void die(char *s);
char * convert(unsigned int addr);

int main(int argc, char *argv[])
{
    struct dhcp_pkt packet;
	struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    char buf[MAXSIZE];
    char* addr;
    unsigned int gate[4];

    if(argc != 2)
    {
        printf("usage: ./executable port\n");
        exit(0);
    }
    int portno = atoi(argv[1]);

    //  init gate
    for(i = 0; i<4; i++)
    {
        gate[i] = 0;
    }
 
 	//	SETUP UDP CONNECTION
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_other, 0, sizeof(si_other));

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(portno);
     
    if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    //printf("client %d\n", portno);
    //printf("---------------------\n");
	memset(buf, '\0', MAXSIZE);

    //  MAKE DHCP REQUEST
    //  129.120.151.94 in hex is 0x8178975E
    packet.siaddr = 0x8178975E;

    //0.0.0.0
    packet.yiaddr = 0x00000000;

    //  randomize transaction id
    srand(time(NULL));
    packet.t_id = rand() % 1000;
    packet.lifetime = 0;

    //  send discover
	if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr*) &si_other, slen) == -1)
    {
        die("sendto()");
    }
    else
    {
        printf("Sending DHCP discover request...\n");
        printPkt(packet, gate);
    }

    //  receive offer
    if (recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, &slen) == -1)
    {
        die("recvfrom()");
    }
    else
    {
        //addr = (char*)packet.yiaddr;
        //parseGate(addr, gate);
        addr = convert(packet.yiaddr);
        parseGate(addr, gate);

        printf("receiving DHCP offer...\n");
        printPkt(packet, gate);
    }

    packet.t_id += 1;

    //  send request
    if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr*) &si_other, slen) == -1)
    {
        die("sendto()");
    }
    else
    {
        printf("Sending DHCP request...\n");
        printPkt(packet, gate);
    }

    //  receive ACK
    if (recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, &slen) == -1)
    {
        die("recvfrom()");
    }
    else
    {
        printf("receiving DHCP ACK...\n");
        printPkt(packet, gate);
    }

    //  keep connection open while lifetime is still up
    printf("> Connection is live... <\n");
    printf("> Staying online until lifetime reaches 0... < \n");
    sleep(packet.lifetime);

    close(s);

	return 0;
}

void die(char *s)
{
    perror(s);
    exit(1);
}


void parseGate(char *addr, unsigned int gate[4])
{
    char *token = addr;
    int i = 0;
    unsigned int temp;

    while(*token)
    {
        if(isdigit(*token))
        {
            temp = strtol(token, &token, 10);

            gate[i] = temp;
            i++;
        }
        else
        {
            token++;
        }
    }
}

//  converts unsigned int to char pointer for parseGate function
char * convert(unsigned int addr)
{
    struct in_addr ip;
    ip.s_addr = addr;
    return(inet_ntoa(ip));
}

void printPkt(struct dhcp_pkt packet, unsigned int gate[4])
{

    printf("---------------------\n");
    printf("Server IP:\t\t0x%08X\t%s\n", packet.siaddr, SERVER);
    printf("Client IP:\t\t0x%08X\t%d.%d.%d.%d\n", packet.yiaddr, gate[3], gate[2], gate[1], gate[0]);
    printf("Transaction ID: \t%d\n", packet.t_id);
    printf("Lifetime:\t\t%d\n", packet.lifetime);
    printf("---------------------\n");
}