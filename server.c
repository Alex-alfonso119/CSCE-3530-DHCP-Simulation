/*
    Alexander Alfonso (aja0167) - CSCE3530: PROGRAM 5
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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

void die(char *s);
void printPkt(struct dhcp_pkt packet, unsigned int gate[]);
void parseGate(char *addr, unsigned int gate[]);
void parseSub(char *addr, unsigned int sub[]);
unsigned int newIP(unsigned int gate[], unsigned int sub[]);

int main(int argc, char *argv[])
{
    struct dhcp_pkt packet;
	struct sockaddr_in si_me, si_other;
    int s, i, slen=sizeof(si_other), recv_len;
    char *gateway;
    char *subnet;

    unsigned int gate[4];
    unsigned int sub[4];
    
    if(argc != 4)
    {
        printf("usage: ./executable <port> <gateway> <subnet_mask>\n");
        exit(0);
    }
    int portno = atoi(argv[1]);
    gateway = argv[2];
    subnet = argv[3];

    /*
    printf("gateway: ");
    scanf("%s", gateway);

    printf("subnet_mask: ");
    scanf("%s", subnet);
    */

    parseGate(gateway, gate); 
    parseSub(subnet, sub);

    //  now gate[] and sub[] each have 4 parts of IP addr

    printf("Waiting for data...\n");
    printf("-------------------\n");

    //	CREATE UDP CONNECTION
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    while(1)
    {

    	// receive DHCP discover
        if ((recv_len = recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        else
        {
        	printf("received DHCP discover request...\n");
            printPkt(packet, gate);
        }

        //  *   i need to calculate offer iP addr
        packet.lifetime = 3600;
        packet.yiaddr = newIP(gate, sub);

        //  send DHCP response
        if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
        else
        {
            printf("sending DHCP offer...\n");
            printPkt(packet, gate);
        }

        // receive DHCP request
        if ((recv_len = recvfrom(s, &packet, sizeof(packet), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        else
        {
            printf("received DHCP request...\n");
            printPkt(packet, gate);
        }

        //  send DHCP ACK
        if (sendto(s, &packet, sizeof(packet), 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
        else
        {
            printf("sending DHCP ACK...\n");
            printPkt(packet, gate);
        }

    }
    
    close(s);
	return 0;
}

void die(char *s)
{
    perror(s);
    exit(1);
}

void printPkt(struct dhcp_pkt packet, unsigned int gate[4])
{

    printf("---------------------\n");
    printf("Server IP:\t\t0x%08X\t%s\n", packet.siaddr, SERVER);
    printf("Client IP:\t\t0x%08X\t%d.%d.%d.%d\n", packet.yiaddr, gate[0], gate[1], gate[2], gate[3]);
    printf("Transaction ID: \t%d\n", packet.t_id);
    printf("Lifetime:\t\t%d\n", packet.lifetime);
    printf("---------------------\n");
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

void parseSub(char *addr, unsigned int sub[4])
{
    char *token = addr;
    int i = 0;
    unsigned int temp = 0;

    while(*token)
    {
        if(isdigit(*token))
        {
            temp = strtol(token, &token, 10);

            sub[i] = temp;
            i++;
        }
        else
        {
            token++;
        }
    }
}

unsigned int newIP(unsigned int gate[4], unsigned int sub[4])
{
    int i;
    unsigned int num1, num2, num3, num4, num5;
    int upperBound = 0;


    for(i=0;i<4;i++)
    {
        if(sub[i] == 255)
        {
            continue;
        }
        else
        {

            //  calculate upperbound of available IP addresses
            if(sub[3] == 0)
            {
                upperBound = 255 - sub[3];
            }
            else
            {
                 upperBound = 255 - sub[3]-1;
            }
           
            //printf("upperbound: %d\n", upperBound);

            if(gate[i]+1 <= upperBound )
            {
                gate[i] = gate[i]+1;
            }
            else
            {
                printf("IP request out of bounds.\n");
                return 0;
            }
            
        }
    }

    //  combine 4 sections into 1 unsigned int
    num1 = gate[0];
    num2 = gate[1];
    num3 = gate[2];
    num4 = gate[3];

    num5 = (num1 << 8) | (num2);
    num5 = (num5 << 8) | (num3);
    num5 = (num5 << 8) | (num4);

    //printf("num5: 0x%08x\n", num5);

    return num5;
}