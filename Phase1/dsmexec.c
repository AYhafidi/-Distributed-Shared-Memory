#include "common_impl.h"

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL; // proc_array = malloc(sizeof dsm proc_t * nombre de machines)

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0; // va aider a traiter les zombies on peut faire autrement

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig) // optionnel
{
  /* on traite les fils qui se terminent */
  /* pour eviter les zombies  */

  int reponse = 1;
  while (reponse > 0)
  {
    reponse = waitpid(0, NULL, WNOHANG);
  }
}


/*******************************************************/
/*********** ATTENTION : BIEN LIRE LA STRUCTURE DU *****/
/*********** MAIN AFIN DE NE PAS AVOIR A REFAIRE *******/
/*********** PLUS TARD LE MEME TRAVAIL DEUX FOIS *******/
/*******************************************************/

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    usage();
  }
  else
  {
    int sfd;
    pid_t pid;
    int num_procs = 0;
    int i;
    char ligne[MAX_STR];
    char buff[MAX_STR];
    int int_buff;
    char hostname[MAX_STR];
    char Port[LENGHT_PID_PORT];
    struct sockaddr_in Dsm;
    socklen_t len = sizeof(Dsm);

    /*Nom de la machine actuelle*/
    if (gethostname(hostname, MAX_STR) < 0)
    {
      perror("Hostname");
      exit(EXIT_FAILURE);
    }
    /* Mise en place d'un traitant pour recuperer les fils zombies*/
    /* XXX.sa_handler = sigchld_handler; */
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sig_action, NULL);

    /* lecture du fichier de machines */
    FILE *fichier = fopen(argv[1], "r");
    if (fichier == NULL)
    {
      perror("Machinefile");
      exit(EXIT_FAILURE);
    }
    /* 1- on recupere le nombre de processus a lancer */
    /* 2- on recupere les noms des machines : le nom de */
    /* la machine est un des elements d'identification */

    while (fscanf(fichier, "%s", ligne) == 1)
    {
      num_procs++;
    }

    /*Infos d'identification des processus dsm (MACHINE_NAME) */
    proc_array = (dsm_proc_t *)malloc(sizeof(dsm_proc_t) * num_procs);
    fseek(fichier, 0, SEEK_SET);
    int j = 0;
    while (fscanf(fichier, "%s", ligne) == 1)
    {
      strcpy(proc_array[j].connect_info.machine, ligne);
      j++;
    }

    /* creation de la socket d'ecoute */

    sfd = handle_bind(hostname);

    // Récuperer le Port

    if (getsockname(sfd, (struct sockaddr *)&Dsm, &len) == -1)
    {
      perror("getsockname");
      exit(EXIT_FAILURE);
    }

    sprintf(Port, "%d", ntohs(Dsm.sin_port));

    /* + ecoute effective */

    if ((listen(sfd, num_procs)) != 0)
    {
      perror("Listen");
      exit(EXIT_FAILURE);
    }

    /*Table de poll*/
    struct pollfd *fds = malloc(2 * num_procs * sizeof(struct pollfd));

    /* creation des fils */
    for (i = 0; i < num_procs; i++)
    {

      /* creation du tube pour rediriger stdout */
      int Pipe_out[2];
      if (pipe(Pipe_out) == -1)
      {
        perror("PIPE :");
        exit(EXIT_FAILURE);
      }
      /* creation du tube pour rediriger stderr */
      int Pipe_err[2];
      if (pipe(Pipe_err) == -1)
      {
        perror("PIPE :");
        exit(EXIT_FAILURE);
      }

      pid = fork();
      if (pid == -1)
        ERROR_EXIT("fork");

      if (pid == 0)
      { /* fils */

        /* redirection stdout */

        dup2(Pipe_out[1], STDOUT_FILENO);
        close(Pipe_out[1]); // fermer l'extremites en écriture aprés l'avoir dupliquer
        close(Pipe_out[0]); // fermer l'extremites en lécture

        /* redirection stderr */

        dup2(Pipe_err[1], STDERR_FILENO);
        close(Pipe_err[1]); // fermer l'extremites en écriture aprés l'avoir dupliquer
        close(Pipe_err[0]); // fermer l'extremites en lécture

        /* Creation du tableau d'arguments pour le ssh */

        char **argv2 = malloc((argc + 4) * sizeof(char *));
        argv2[0] = "ssh";
        argv2[1] = proc_array[i].connect_info.machine;
        argv2[2] = "dsmwrap"; // dsmwrap
        argv2[3] = hostname;  // nom de la machine actuelle (dsmexec)
        argv2[4] = Port;      // le port de cette machine (dsmexec)
        for (i = 5; i < argc + 4; i++)
        {
          argv2[i] = argv[i - 3];
        }
        argv2[argc + 4] = NULL;

        /* jump to new prog : */
        //dsmwrap est faire l'etape d'interconnection entre le machines . notament on doit faire ssh de dsmwrap qui dispatchent les infos de connexion
        // notamment le port et l'addresse IP.

        execvp("ssh", argv2);
      }
      else if (pid > 0)
      { /* pere */

        /*Poll informations recu de stdout de processus i*/
        fds[2 * i].fd = Pipe_out[0];
        fds[2 * i].events = POLLIN;

        /* Poll informations recu de stderr de processus i*/
        fds[2 * i + 1].fd = Pipe_err[   0];
        fds[2 * i + 1].events = POLLIN;

        /* fermeture des extremites non utiles */
        close(Pipe_err[1]);
        close(Pipe_out[1]);

        /*Les informations d'identification des processus dsm (PID - RANK)*/
        proc_array[i].connect_info.rank = i;

        num_procs_creat++;
      }
    }

    for (i = 0; i < num_procs_creat; i++)
    {
      int connfd = 0;
      do
      {
        /* on accepte les connexions des processus dsm */
        connfd = accept(sfd, (struct sockaddr *)&Dsm, &len);
      } while (connfd == -1);

      /*  On recupere le nom de la machine distante */
      /* les ._STR */
      Read_string(connfd, buff);
      int rank = get_rank(buff, proc_array, num_procs);

      /*Les informations d'identification des processus dsm (Fd)*/
      proc_array[rank].connect_info.fd = connfd;

      /* On recupere le pid du processus distant  (optionnel)*/
      Read_int(connfd, &int_buff);
      proc_array[rank].pid = int_buff;

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
      Read_int(connfd, &int_buff);
      proc_array[rank].connect_info.port_num = int_buff;
   } 
      //printf("1\n");
      //printf("machine : %s et  rang :  %i \n",proc_array[i].connect_info.machine,i);
      /* cf code de dsmwrap.c */

      /***********************************************************/
      /********** ATTENTION : LE PROTOCOLE D'ECHANGE *************/
      /********** DECRIT CI-DESSOUS NE DOIT PAS ETRE *************/
      /********** MODIFIE, NI DEPLACE DANS LE CODE   *************/
      /***********************************************************/
for (i = 0; i < num_procs_creat; i++)
    {
      /* 1- envoi du nombre de processus aux processus dsm*/
      /* On envoie cette information sous la forme d'un ENTIER */
      /* (IE PAS UNE CHAINE DE CARACTERES */
      
      Write_int(proc_array[i].connect_info.fd, num_procs_creat);

      /* 2- envoi des rangs aux processus dsm */
      /* chaque processus distant ne reçoit QUE SON numéro de rang */
      /* On envoie cette information sous la forme d'un ENTIER */
      /* (IE PAS UNE CHAINE DE CARACTERES */

      Write_int(proc_array[i].connect_info.fd, proc_array[i].connect_info.rank);

      /* 3- envoi des infos de connexion aux processus */
      /* Chaque processus distant doit recevoir un nombre de */
      /* structures de type dsm_proc_conn_t égal au nombre TOTAL de */
      /* processus distants, ce qui signifie qu'un processus */
      /* distant recevra ses propres infos de connexion */
      /* (qu'il n'utilisera pas, nous sommes bien d'accords). */
      for (j = 0; j < num_procs; j++)
      {
        size_t sent = 0;
        size_t to_send = sizeof(dsm_proc_conn_t);
        int r = 0;
        do
        {
          do
          {
            r = write(proc_array[i].connect_info.fd, (char *)&(proc_array[j].connect_info) + sent, to_send - sent);
          } while (-1 == r);
          sent += r;
        } while (sent != to_send);
      }
    }

    /***********************************************************/
    /********** FIN DU PROTOCOLE D'ECHANGE DES DONNEES *********/
    /********** ENTRE DSMEXEC ET LES PROCESSUS DISTANTS ********/
    /***********************************************************/

    /* gestion des E/S : on recupere les caracteres */
    /* sur les tubes de redirection de stdout/stderr */

    memset(buff, 0, MAX_STR);
    while (1)
    {
      /*je recupere les infos sur les tubes de redirection
            jusqu'à ce qu'ils soient inactifs (ie fermes par les
            processus dsm ecrivains de l'autre cote ...)*/
      int r = poll(fds, 2 * num_procs, -1);
      if (r < 0)
      {
        perror("Poll");
        exit(EXIT_FAILURE);
      }

      for (i = 0; i < num_procs; i++)
      {

        /*Sorties Standard*/

        if (fds[2 * i].revents & POLLIN)
        {

          if (read(fds[2 * i].fd, buff, MAX_STR) < 0)
          {
            perror("Reading");
            exit(EXIT_FAILURE);
          }

          fprintf(stdout, "[ Proc %d : %s : stdout ] : %s\n", i, proc_array[i].connect_info.machine, buff);

          fds[2 * i].revents = 0;
        }

        memset(buff, 0, MAX_STR);

        /*Sorties D'erreur*/

        if (fds[2 * i + 1].revents & POLLIN)
        {

          if (read(fds[2 * i + 1].fd, buff, MAX_STR) < 0)
          {
            perror("Reading");
            exit(EXIT_FAILURE);
          }
          fprintf(stdout, "[ Proc %d : %s : stderr ] : %s\n", i, proc_array[i].connect_info.machine, buff);

          fds[2 * i + 1].revents = 0;
        }
      }
    };

    /* on attend les processus fils */

    for (i=0; i<num_procs_creat;i++){
      waitpid(proc_array[i].pid,NULL,0);
    }
    /* on ferme les descripteurs proprement */

    for (i=0; i<num_procs_creat;i++){
      close(proc_array[i].connect_info.fd);
      close(fds[2 * i].fd);
      close(fds[2 * i+1].fd);
    }

    // fermeture du fichier
    
    fclose(fichier);

    /* on ferme la socket d'ecoute */
    close(sfd);
    free(proc_array);
  }
  exit(EXIT_SUCCESS);
}
