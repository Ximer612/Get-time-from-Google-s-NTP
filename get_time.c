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
    const unsigned char leap_version_mode;
    const unsigned char stratum;
    const unsigned char poll;
    const unsigned char precision;
    const unsigned int root_delay;

    const unsigned int root_dispersion;
    const char rfid[4];

    const unsigned long long ref_timestamp;
    const unsigned long long original_timestamp;
    const unsigned long long receive_timestamp;
    const unsigned long long transmit_timestamp;
} ntp_packet;

void print_ntp_packet(ntp_packet packet)
{
    MAGENTA_PRINT("Leap => %d, Version => %d, Mode => %d", packet.leap_version_mode >> 6, packet.leap_version_mode >> 3 & 0xFFF, packet.leap_version_mode & 0xF);
    MAGENTA_PRINT("stratum => %d", packet.stratum);
    MAGENTA_PRINT("poll => %d", packet.poll);
    MAGENTA_PRINT("precision => %d", packet.precision);
    MAGENTA_PRINT("root_delay => %d", packet.root_delay);
    MAGENTA_PRINT("root_dispersion => %d", packet.root_dispersion);
    MAGENTA_PRINT("reference identifier => %s", packet.rfid);
    MAGENTA_PRINT("reference timestamp => %llu", packet.ref_timestamp);
    MAGENTA_PRINT("original timestamp => %llu", packet.original_timestamp);
    MAGENTA_PRINT("receive timestamp => %llu", packet.receive_timestamp);
    MAGENTA_PRINT("transmit timestamp => %llu", packet.transmit_timestamp);
}

void print_ntp_packet_inline(ntp_packet packet)
{
    SET_BLUE_PRINT();
    printf("Leap => %d, Version => %d, Mode => %d", packet.leap_version_mode >> 6, packet.leap_version_mode >> 3 & 0xFFF, packet.leap_version_mode & 0xF);
    printf(", stratum => %d", packet.stratum);
    printf(", poll => %d", packet.poll);
    printf(", precision => %d", packet.precision);
    printf(", root_delay => %d", packet.root_delay);
    printf(", root_dispersion => %d", packet.root_dispersion);
    printf(", reference identifier => %s", packet.rfid);
    printf(", reference timestamp => %llu", packet.ref_timestamp);
    printf(", original timestamp => %llu", packet.original_timestamp);
    printf(", receive timestamp => %llu", packet.receive_timestamp);
    printf(", transmit timestamp => %llu \n", packet.transmit_timestamp);
    SET_DEFAULT_PRINT();
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
    size_t received_bytes = recvfrom(s, (char*)&recv_packet, sizeof(recv_packet), 0,
                                      (struct sockaddr*)&sender, &sender_len);
    print_ntp_packet(recv_packet);

    char outstr[128];
    time_t t;
    struct tm tmp;

    localtime_s(&tmp,&now);
    strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", &tmp);
    YELLOW_PRINT("My computer's today is %s", outstr);

    time_t recv_time = (time_t)((recv_packet.receive_timestamp >> 32));

    localtime_s(&tmp,&recv_time);
    strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", &tmp);
    YELLOW_PRINT("Server google's today is %s", outstr);

    //Python result => (36, 1, 0, 236, 0, 7, b'GOOG', 16810710409316310953, 0, 16810710409316310954, 16810710409316310956)

    return 0;
}