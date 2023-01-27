#include "dsm_impl.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */

/* Socket Bind 
int handle_bind(char *Adresse)
{
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(Adresse, NULL, &hints, &result) != 0)
	{
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = frp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		close(sfd);
	}
	if (rp == NULL)
	{
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int handle_connect(char *Adresse, char *Port)
{
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(Adresse, Port, &hints, &result) != 0)
	{
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
		{
			break;
		}
		close(sfd);
	}
	if (rp == NULL)
	{
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}
*/
/*********** Write char **************/

void Write_string(int sockfd, char *buff)
{
	size_t sent = 0;
	size_t to_send = MAX_STR;
	do
	{
		size_t r = write(sockfd, buff + sent, to_send - sent);
		if (r != -1)
			sent += r;
	} while (sent != to_send);
}

/*********** Write int **************/
void Write_int(int sockfd, int tmp)
{
	size_t sent = 0;
	size_t to_send = sizeof(int);
	int int_buff = tmp;
	do
	{
		size_t r = write(sockfd, (char *)&int_buff + sent, to_send - sent);
		if (r != -1)
			sent += r;
	} while (sent != to_send);
}

/*********** Read char ***********/
void Read_string(int sockfd, char *buff)
{
	size_t sent = 0;
	size_t to_send = MAX_STR;
	memset(buff, 0, MAX_STR);
	do
	{
		size_t r = read(sockfd, buff + sent, to_send - sent);
		if (r != -1)
			sent += r;
	} while (sent != to_send);
}

/*********** Read Int ***********/
void Read_int(int sockfd, int *tmp)
{
	size_t sent = 0;
	size_t to_send = sizeof(int);
	do
	{
		size_t r = read(sockfd, (char *)tmp + sent, to_send - sent);
		if (r != -1)
			sent += r;
	} while (sent != to_send);
}

/* le rang*/


