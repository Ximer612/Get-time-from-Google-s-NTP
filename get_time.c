#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <time.h>
#include <string.h>
#define DEBUG_PRINT
#include <console_utilities.h>

typedef struct ntp_packet
{
    unsigned char leap_version_mode;
    unsigned char stratum;
    unsigned char poll;
    unsigned char precision;
    unsigned int root_delay;

    unsigned int root_dispersion;
    char rfid[4];

    unsigned long long ref_timestamp;
    unsigned long long original_timestamp;
    unsigned long long receive_timestamp;
    unsigned long long transmit_timestamp;
} ntp_packet;

unsigned int to_le_int(const unsigned int value)
{
    unsigned int byte0 = value << 24;
    unsigned int byte1 = (value & 0xFF00) << 8;
    unsigned int byte2 = (value & 0xFF0000) >> 8;
    unsigned int byte3 = (value & 0xFF000000) >> 24;

    return byte0 | byte1 | byte2 | byte3;
}

unsigned long long to_le_long(const unsigned long long value) {
    unsigned long long byte0 = value << 56;
    unsigned long long byte1 = (value & 0xFF00) << 40;
    unsigned long long byte2 = (value & 0xFF0000) << 24;
    unsigned long long byte3 = (value & 0xFF000000) << 8;
    unsigned long long byte4 = (value & 0xFF00000000) >> 8;
    unsigned long long byte5 = (value & 0xFF0000000000) >> 24;
    unsigned long long byte6 = (value & 0xFF000000000000) >> 40;
    unsigned long long byte7 = (value >> 56);

    return byte0 | byte1 | byte2 | byte3 | byte4 | byte5 | byte6 | byte7;
}

void print_ntp_packet(ntp_packet packet)
{
    MAGENTA_PRINT("Leap => %d, Version => %d, Mode => %d", packet.leap_version_mode >> 6, packet.leap_version_mode >> 3 & 0xFFF, packet.leap_version_mode & 0xF);
    MAGENTA_PRINT("stratum => %d", packet.stratum);
    MAGENTA_PRINT("poll => %d", packet.poll);
    MAGENTA_PRINT("precision => %d", packet.precision);
    MAGENTA_PRINT("root_delay => %d", packet.root_delay);
    MAGENTA_PRINT("root_dispersion => %d", packet.root_dispersion);
    MAGENTA_PRINT("reference identifier => %.4s", packet.rfid);
    MAGENTA_PRINT("reference timestamp => %llu", packet.ref_timestamp);
    MAGENTA_PRINT("original timestamp => %llu", packet.original_timestamp);
    MAGENTA_PRINT("receive timestamp => %llu", packet.receive_timestamp);
    MAGENTA_PRINT("transmit timestamp => %llu",packet.transmit_timestamp);
}

int main(int argc, char **argv)
{

#ifdef _WIN32
    // this part is only required on Windows: it initializes the Winsock2 dll
    WSADATA wsa_data;
    if (WSAStartup(0x0202, &wsa_data))
    {
        RED_PRINT("unable to initialize winsock2");
        return -1;
    }

#endif

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
    {
        RED_PRINT("unable to initialize the UDP socket");
        return -1;
    }
    GREEN_PRINT("socket %d created", s);

    const int leap = 0;
    const int version = 4;
    const int mode = 3;

    const time_t now = time(NULL);
    CYAN_PRINT("Actual time => %llu", now);

    ntp_packet sender_packet = {leap << 6 | version << 3 | mode,0,0,0,0,0,"\0\0\0\0",0,(unsigned long long)now,0,0
    };
    
    struct sockaddr_in sin;
    inet_pton(AF_INET, "216.239.35.12", &sin.sin_addr); // this will create a big endian 32 bit address
    sin.sin_family = AF_INET;
    sin.sin_port = htons(123);

    int sent_bytes = sendto(s, (char *)&sender_packet, sizeof(ntp_packet), 0, (struct sockaddr *)&sin, sizeof(sin));
    GREEN_PRINT("sent %d bytes via UDP", sent_bytes);

    ntp_packet recv_packet;
    memset(&recv_packet,0,sizeof(ntp_packet));
    
    struct sockaddr_in sender;
    int sender_len = sizeof(sender);
    size_t received_bytes = recvfrom(s, (char*)&recv_packet, sizeof(ntp_packet), 0,
                                      (struct sockaddr*)&sender, &sender_len);

    recv_packet.root_delay = to_le_int(recv_packet.root_delay);
    recv_packet.root_dispersion = to_le_int(recv_packet.root_dispersion);
    recv_packet.ref_timestamp = to_le_long(recv_packet.ref_timestamp);
    recv_packet.original_timestamp = to_le_long(recv_packet.original_timestamp);
    recv_packet.receive_timestamp = to_le_long(recv_packet.receive_timestamp);
    recv_packet.transmit_timestamp = to_le_long(recv_packet.transmit_timestamp);
    
    print_ntp_packet(recv_packet);

    char outstr[128];
    struct tm tmp;

    localtime_s(&tmp,&now);
    strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", &tmp);
    YELLOW_PRINT("My computer's today is %s", outstr);

    int leap_years_count = (tmp.tm_year - 55)/4; //1954
    time_t recv_time = (time_t)((recv_packet.receive_timestamp >> 32)  - ((unsigned int)365 * 3600 * 24 * 70) - ((unsigned int)leap_years_count * 3600 * 24));

    localtime_s(&tmp,&recv_time);

    strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", &tmp);
    YELLOW_PRINT("Server google's today is %s", outstr);

    //Debug create file
    FILE* file_to_write;
    fopen_s(&file_to_write,"ntp_packet.txt", "w");

    if(file_to_write == NULL)
    {
        printf("Unable to create file.\n");
        return -1;
    }

    fwrite((char*)&recv_packet,sizeof(char),sizeof(ntp_packet),file_to_write);
    fclose(file_to_write);

    return 0;
}