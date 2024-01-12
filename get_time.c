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
    const char *reference_identifier = "\0\0\0\0";

    ntp_packet sender_packet;
    sender_packet.leap_version_mode = leap << 6 | version << 3 | mode;
    strcpy_s(sender_packet.rfid, sizeof(sender_packet.rfid), reference_identifier);

    const int now = (int)time(NULL);

    CYAN_PRINT("Actual time => %d", now);

    struct sockaddr_in sin;
    inet_pton(AF_INET, "216.239.35.12", &sin.sin_addr); // this will create a big endian 32 bit address
    sin.sin_family = AF_INET;
    sin.sin_port = htons(123); // converts 9999 to big endian

    int sent_bytes = sendto(s, (char *)&sender_packet, sizeof(ntp_packet), 0, (struct sockaddr *)&sin, sizeof(sin));
    GREEN_PRINT("sent %d bytes via UDP", sent_bytes);

    char buffer[sizeof(ntp_packet)];
    ntp_packet recv_packet;

    for (;;)
    {
        struct sockaddr_in sender_in;
        int sender_in_size = sizeof(sender_in);
        int len = recvfrom(s, (char *)&recv_packet, sizeof(ntp_packet), 0, (struct sockaddr *)&sender_in, &sender_in_size);
        if (len > 0)
        {
            char addr_as_string[64];
            inet_ntop(AF_INET, &sender_in.sin_addr, addr_as_string, 64);
            GREEN_PRINT("received %d bytes from %s:%d", len, addr_as_string, ntohs(sender_in.sin_port));
            break;
        }
    }

    MAGENTA_PRINT("Leap => %d, Version => %d, Mode => %d", recv_packet.leap_version_mode >> 6, recv_packet.leap_version_mode >> 3 & 0xFFF, recv_packet.leap_version_mode & 0xF);
    MAGENTA_PRINT("stratum => %d", recv_packet.stratum);
    MAGENTA_PRINT("poll => %d", recv_packet.poll);
    MAGENTA_PRINT("precision => %d", recv_packet.precision);
    MAGENTA_PRINT("root_delay => %d", recv_packet.root_delay);
    MAGENTA_PRINT("root_dispersion => %d", recv_packet.root_dispersion);
    MAGENTA_PRINT("reference identifier => %s", recv_packet.rfid);
    MAGENTA_PRINT("reference timestamp => %llu", recv_packet.ref_timestamp);
    MAGENTA_PRINT("original timestamp => %llu", recv_packet.original_timestamp);
    MAGENTA_PRINT("receive timestamp => %llu", recv_packet.receive_timestamp);
    MAGENTA_PRINT("transmit timestamp => %llu", recv_packet.transmit_timestamp);

    return 0;
}