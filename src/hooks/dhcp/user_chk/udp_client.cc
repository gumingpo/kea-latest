/*
    Simple udp client
*/
#include <udp_client.h>
extern "C" {
	int udp_send(char *msg, int msglen)
	{
		struct sockaddr_in si_other;
		int slen=sizeof(si_other);
		int sock;

		if(msg == 0 || msg[0] == '\0' || msglen <= 0) {
            return -1;
		}
		if ( (sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		{
			perror("failed to create socket");
			return -2;
		}

		memset((char *) &si_other, 0, slen);
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(PORT);

		if (inet_aton(SERVER , &si_other.sin_addr) == 0)
		{
			perror("failed to convert ip address\n");
			return -3;
		}

		//send the message
		//ssize_t sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
		if (sendto(sock, msg, msglen , 0 , (struct sockaddr *) &si_other, slen)==-1)
		{
			perror("failed to send message");
			return errno;
		}

		close(sock);
		return 0;
	}
}
