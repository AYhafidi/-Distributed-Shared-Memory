#include "dsm_impl.h"

int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */
int DEMENDING_PAGE=0;  /* prêt pour demander une page*/
int DSM_READY_TO_DISC=0;  /*Nombre de processus dsm prêt à ce terminer*/

struct pollfd *fds = NULL;
static dsm_proc_conn_t *procs = NULL;
static dsm_page_info_t table_page[PAGE_NUMBER];
static pthread_t comm_daemon;



																													/* initialisation de tableau pour le poll*/
void fds_init(){
	fds=malloc(DSM_NODE_NUM*sizeof(struct pollfd));
	int i;
	for(i=0;i<DSM_NODE_NUM;i++){
			fds[i].fd=procs[i].fd;
			fds[i].events=POLLIN;
			fds[i].revents=0;
	}
}
/*Gérer la Connection */

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
		sfd=-1;
	}

	freeaddrinfo(result);
	return sfd;
}

/* indique l'adresse de debut de la page de numero numpage */
static char *num2address(int numpage)
{
	char *pointer = (char *)(BASE_ADDR + (numpage * (PAGE_SIZE)));

	if (pointer >= (char *)TOP_ADDR)
	{
		fprintf(stderr, "[%i] Invalid address !\n", DSM_NODE_ID);
		return NULL;
	}
	else
		return pointer;
}

/* cette fonction permet de recuperer un numero de page */
/* a partir  d'une adresse  quelconque */
static int address2num(char *addr)
{
	return (((intptr_t)(addr - BASE_ADDR)) / (PAGE_SIZE));
}

/* cette fonction permet de recuperer l'adresse d'une page */
/* a partir d'une adresse quelconque (dans la page)        */
static char *address2pgaddr(char *addr)
{
	return (char *)(((intptr_t)addr) & ~(PAGE_SIZE - 1));
}

/* fonctions pouvant etre utiles */
static void dsm_change_info(int numpage, dsm_page_state_t state, dsm_page_owner_t owner)
{
	if ((numpage >= 0) && (numpage < PAGE_NUMBER))
	{
		if (state != NO_CHANGE)
			table_page[numpage].status = state;
		if (owner >= 0)
			table_page[numpage].owner = owner;
		return;
	}
	else
	{
		fprintf(stderr, "[%i] Invalid page number !\n", DSM_NODE_ID);
		return;
	}
}

static dsm_page_owner_t get_owner(int numpage)
{
	return table_page[numpage].owner;
}

static dsm_page_state_t get_status(int numpage)
{
	return table_page[numpage].status;
}
/* Allocation d'une nouvelle page */
static void dsm_alloc_page(int numpage)
{
	char *page_addr = num2address(numpage);
	mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return;
}

/* Changement de la protection d'une page */
static void dsm_protect_page(int numpage, int prot)
{
	char *page_addr = num2address(numpage);
	mprotect(page_addr, PAGE_SIZE, prot);
	return;
}

static void dsm_free_page(int numpage)
{
	char *page_addr = num2address(numpage);
	munmap(page_addr, PAGE_SIZE);
	return;
}


/* Traitant des Requêtes */

void Req_handler(int sock_fd) {
				dsm_req_t dsm_request;
				int i;
				dsm_recv(sock_fd,&dsm_request,sizeof(dsm_req_t));
				switch (dsm_request.request) {
								case DSM_REQ:
											dsm_free_page(dsm_request.page_num);
											dsm_request.source=DSM_NODE_ID;
											dsm_request.request=DSM_PAGE;
											dsm_send(sock_fd,&dsm_request,sizeof(dsm_req_t));
											break;
								case DSM_PAGE:
											dsm_alloc_page(dsm_request.page_num);
											dsm_request.source=DSM_NODE_ID;
											dsm_request.request=DSM_NREQ;
											dsm_change_info(dsm_request.page_num,NO_CHANGE,DSM_NODE_ID);
											for (i=0;i<DSM_NODE_NUM;i++){
															if (i!=DSM_NODE_ID){
																			dsm_send(procs[i].fd,&dsm_request,sizeof(dsm_req_t));
																		}
																	}
											break;

											DEMENDING_PAGE=0;

								case DSM_NREQ:
											dsm_change_info(dsm_request.page_num,NO_CHANGE,dsm_request.source);
											break;

								case DSM_FINALIZE:
											DSM_READY_TO_DISC++;
											break;
								default:

								break;
							}
}

static void *dsm_comm_daemon( void *arg)
{
		int	i;
		int r;

		while(1)
			 {
					if (-1==(r==poll(fds,DSM_NODE_NUM,-1))){
						perror("poll");
						exit(EXIT_FAILURE);
					}



					for (i = 0; i < DSM_NODE_NUM; i++) {

						if (fds[i].revents&POLLIN){

								Req_handler(fds[i].fd);
								fds[i].revents=0;

							}
						}
						if (DSM_READY_TO_DISC==DSM_NODE_NUM){
							pthread_exit(NULL);
						}
     }
   return NULL;
}

static int dsm_send(int dest, void *buf, size_t size)
{
	size_t sent = 0;
	do
	{
		size_t r = write(dest, buf + sent, size - sent);
		if (r != -1)
			sent += r;
	} while (sent != size);
	return 0;
}

static int dsm_recv(int from, void *buf, size_t size)
{
	/* a completer */
	size_t sent = 0;
	do
	{
		size_t r = read(from, buf + sent, size - sent);
		if (-1 == r)
		{
			perror("Write");
			exit(EXIT_FAILURE);
		}
		sent += r;
	} while (size != sent);

	return 0;
}

static void dsm_handler(int numpage)
{
   /* A modifier */
	 if (DEMENDING_PAGE==0){
	 dsm_req_t dsm_request;
	 dsm_request.source=DSM_NODE_ID;
	 dsm_request.page_num=numpage;
	 dsm_request.request=DSM_REQ;
	 dsm_send(procs[get_owner(numpage)].fd,&dsm_request,sizeof(dsm_req_t));
	 DEMENDING_PAGE=1;
 	}
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context)
{
	/* A completer */
	/* adresse qui a provoque une erreur */
	void *addr = info->si_addr;
	/* Si ceci ne fonctionne pas, utiliser a la place :*/
	/*
   #ifdef __x86_64__
   void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
   #elif __i386__
   void *addr = (void *)(context->uc_mcontext.cr2);
   #else
   void  addr = info->si_addr;
   #endif
   */
	/*
   pour plus tard (question ++):
   dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;
  */
	/* adresse de la page dont fait partie l'adresse qui a provoque la faute */
	void *page_addr = (void *)(((unsigned long)addr) & ~(PAGE_SIZE - 1));

	if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR))
	{
		dsm_handler(address2num(addr));
	}
	else
	{
		/* SIGSEGV normal : ne rien faire*/
	}

}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char *argv[])
{
	struct sigaction act;
	int index;

	/* Récupération de la valeur des variables d'environnement */
	/* DSMEXEC_FD et MASTER_FD*/
	int dsmexec_fd = atoi(getenv("DSMEXEC_FD"));
	int master_fd = atoi(getenv("MASTER_FD"));

	/* reception du nombre de processus dsm envoye */
	/* par le lanceur de programmes (DSM_NODE_NUM) */
	dsm_recv(dsmexec_fd, &DSM_NODE_NUM, sizeof(int));
	/* reception de mon numero de processus dsm envoye */
	/* par le lanceur de programmes (DSM_NODE_ID)      */
	dsm_recv(dsmexec_fd, &DSM_NODE_ID, sizeof(int));
	/* reception des informations de connexion des autres */
	/* processus envoyees par le lanceur :                */
	/* nom de machine, numero de port, etc.               */

	procs = malloc(DSM_NODE_NUM * sizeof(dsm_proc_conn_t));
	int i;
	for (i = 0; i < DSM_NODE_NUM; i++)
	{
		dsm_recv(dsmexec_fd, &procs[i], sizeof(dsm_proc_conn_t));
	}
	procs[DSM_NODE_ID].fd = dsmexec_fd;

	/* initialisation des connexions              */
	/* avec les autres processus : connect/accept */
	if (-1 == listen(master_fd, DSM_NODE_NUM))
	{
		perror("Listen");
		exit(EXIT_FAILURE);
	}

	for (index = DSM_NODE_ID; index < DSM_NODE_NUM - 1; index++)
	{
		int sfd;
		do
		{
			sfd = accept(master_fd, NULL, NULL);
		} while (-1 == sfd);
		int rank;
		dsm_recv(sfd, &rank, sizeof(int));
		procs[rank].fd = sfd;
	}
	for (index = 0; index < DSM_NODE_ID; index++)
	{
		char Port[15];
		sprintf(Port, "%d", procs[index].port_num);
		do
		{
			procs[index].fd = handle_connect(procs[index].machine, Port);
		} while (-1 == procs[index].fd);
		dsm_send(procs[index].fd,&DSM_NODE_ID,sizeof(int));
		}
		fds_init();
   /* Allocation des pages en tourniquet */
   for(index = 0; index < PAGE_NUMBER; index ++){
     if ((index % DSM_NODE_NUM) == DSM_NODE_ID)
       dsm_alloc_page(index);
     dsm_change_info( index, WRITE, index % DSM_NODE_NUM);
   }

   /* mise en place du traitant de SIGSEGV */
   act.sa_flags = SA_SIGINFO;
   act.sa_sigaction = segv_handler;
   sigaction(SIGSEGV, &act, NULL);

   /* creation du thread de communication           */
   /* ce thread va attendre et traiter les requetes */
   /* des autres processus                         */
   pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);



   /* Adresse de début de la zone de mémoire partagée */
   return ((char *)BASE_ADDR);
}



void dsm_finalize(void)
{
	DSM_READY_TO_DISC++;
	 dsm_req_t dsm_request;
	 dsm_request.source=DSM_NODE_ID;
	 dsm_request.request=DSM_FINALIZE;
	 int i;
	 for (i=0;i<DSM_NODE_NUM;i++){
		 if (i!=DSM_NODE_ID){
		 		dsm_send(procs[i].fd,&dsm_request,sizeof(dsm_request));
			}
	 }



   /* terminer correctement le thread de communication */
	 pthread_join(comm_daemon,NULL);

	 /*libérer les mallocs*/
	  free(fds);
	  free(procs);

		/* fermer proprement les connexions avec les autres processus */
		for (i=0;i<DSM_NODE_NUM;i++){
			if (i==DSM_NODE_ID){
				 close(procs[DSM_NODE_ID].fd);
				 close(procs[DSM_NODE_ID].fd_for_exit);
			 }
			 else{
				 close(procs[i].fd);
		 	}
		}
	return;
	

}
