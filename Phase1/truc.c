#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>

#define MAX_STR  (1024)
typedef char maxstr_t[MAX_STR];


struct dsm_proc_conn  {
   int      rank;
   maxstr_t machine;
   int      port_num;
   int      fd;
   int      fd_for_exit; /* special */ // le numero de descripteur pour sortir proprement
};

typedef struct dsm_proc_conn dsm_proc_conn_t;

struct dsm_proc {  // cette structure est modifiable
  pid_t pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;



int main(int argc, char *argv[] , char **envp)
{
   int fd;
   int i;
   char str[1024];
   char exec_path[2048];
   char *wd_ptr = getcwd(str,1024);

                                        /*  recupérer la variable d'environnemnt */
   int dsmexec_fd = atoi(getenv("DSMEXEC_FD"));
   int dsmlstn_fd = atoi(getenv("MASTER_FD"));

                                                                    /*Numéro de processus*/
   size_t recvd=0;
   size_t to_recv=sizeof(int);
   int num_procs;

   do{
 			size_t r=read(dsmexec_fd,(char *)&num_procs+recvd,to_recv-recvd);
 			if (r!=-1)
 			 	recvd+=r;
 			}while (recvd!=to_recv);

                                                                    /*Numéro de rang*/
    recvd=0;
    int rank;
    do{
       size_t r=read(dsmexec_fd,(char *)&rank+recvd,to_recv-recvd);
       if (r!=-1)
         recvd+=r;
       }while (recvd!=to_recv);

   
    dsm_proc_t * proc_array = (dsm_proc_t *) malloc(sizeof(dsm_proc_t ) * num_procs );
    
    to_recv= num_procs *sizeof(  dsm_proc_t );

    if (-1==recv(dsmexec_fd,proc_array,to_recv,0)) {
    perror("Recieve");
    exit(EXIT_FAILURE);

    }
    printf("sent rank : %d\n",proc_array[2].connect_info.rank);
  

   fprintf(stdout, "\n\t<==========================INFOS==============================>\n\n");
   printf("Le nombre de processus est : %d\nrank : %d\n",num_procs,rank);
   fprintf(stdout, "\n\t<============================================================>\n");

   fprintf(stdout,"Working dir is %s\n",wd_ptr);

   fprintf(stdout,"Number of args : %i\n", argc);
   for(i= 0; i < argc ; i++)
     fprintf(stderr,"arg[%i] : %s\n",i,argv[i]);

   sprintf(exec_path,"%s/%s",wd_ptr,"titi");
   fd = open(exec_path,O_RDONLY);
   if(fd == -1) perror("open");
   fprintf(stdout,"=============== Valeur du descripteur : %i\n",fd);
   fprintf(stderr, "\n\t<============================================================>\n");
   fflush(stdout);
   fflush(stderr);
   return 0;
}
