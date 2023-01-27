#include "common_impl.h"

int main(int argc, char **argv)
{

  int connfd;
  int sfd;

  /* processus intermediaire pour "nettoyer" */
  /* la liste des arguments qu'on va passer */
  /* a la commande a executer finalement  */

  char *hostname = argv[1];
  char *port = argv[2];
  char **argv_exec = malloc((argc - 2) * sizeof(char *));
  int i;
  for (i = 0; i < argc - 2; i++)
  {
    argv_exec[i] = argv[i + 3];
  }

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */
  connfd = handle_connect(hostname, port);
  if (connfd < 0)
  {
    perror("Connect");
    exit(EXIT_FAILURE);
  }

  /*Le nom de la machine*/
  char Hostname[MAX_STR];
  if (gethostname(Hostname, MAX_STR) < 0)
  {
    perror("Hostname");
    exit(EXIT_FAILURE);
  }

  /* Envoi du nom de machine au lanceur */
  Write_string(connfd, Hostname);

  /* Envoi du pid au lanceur (optionnel) */

  pid_t pid = getpid();
  Write_int(connfd, pid);

  /* Creation de la socket d'ecoute pour les */
  /* connexions avec les autres processus dsm */
  sfd = handle_bind(Hostname);

  if (listen(sfd, 10))
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  /* Envoi du numero de port au lanceur */
  /* pour qu'il le propage à tous les autres */
  /* processus dsm */

  struct sockaddr_in Dsm;
  socklen_t len = sizeof(Dsm);

  if (getsockname(sfd, (struct sockaddr *)&Dsm, &len) == -1)
  {
    perror("getsockname");
    exit(EXIT_FAILURE);
  }

  int Port = ntohs(Dsm.sin_port);
  Write_int(connfd, Port);

  /* on execute la bonne commande */
  /* attention au chemin à utiliser ! */

  char fd_str[MAX_STR];
  sprintf(fd_str, "%d", connfd);
  setenv("DSMEXEC_FD", fd_str, 0);

  memset(fd_str, 0, MAX_STR);

  sprintf(fd_str, "%d", sfd);
  setenv("MASTER_FD", fd_str, 0);

  execvp(argv_exec[0], argv_exec);

  /************** ATTENTION **************/
  /* vous remarquerez que ce n'est pas   */
  /* ce processus qui récupère son rang, */
  /* ni le nombre de processus           */
  /* ni les informations de connexion    */
  /* (cf protocole dans dsmexec)         */
  /***************************************/

  return 0;
}
