#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>


/* autres includes (eventuellement) */

#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/**************************************************************/
/****************** DEBUT DE PARTIE NON MODIFIABLE ************/
/**************************************************************/

#define MAX_STR  (1024)
#define DSM_NODE_NUM 10
typedef char maxstr_t[MAX_STR];
#define LENGHT_PID_PORT 10

/* definition du type des infos */
/* de connexion des processus dsm */

// on envoie n fois cette structure. maxstr_t machine = nom du machine
struct dsm_proc_conn  {
   int      rank;
   maxstr_t machine;
   int      port_num;
   int      fd;
   int      fd_for_exit; /* special */ // le numero de descripteur pour sortir proprement
};

typedef struct dsm_proc_conn dsm_proc_conn_t;

/**************************************************************/
/******************* FIN DE PARTIE NON MODIFIABLE *************/
/**************************************************************/

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {  // cette structure est modifiable
  pid_t pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

                                                              /*Function Header*/
int handle_bind(char *Adresse);
void Read_string(int sockfd,char *buff);
void Read_int(int sockfd,int *tmp);
int handle_connect(char *Adresse,char *Port);
void Write_string(int sockfd,char *buff);
void Write_int(int sockfd,int tmp);
int get_rank(char *hostname,dsm_proc_t *proc_array,int num_procs);
