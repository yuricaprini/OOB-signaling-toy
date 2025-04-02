#include "macros.h"
#include "comsock.h"
#include "internals.h"
#include "endian.h"
#include "compipe.h"

#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define SOCK_NAME "OOB-server-%d"

typedef struct args
{

	int id;
	int fd_acc;
	int scrittore;

} args_t;

typedef struct tidlist
{
	pthread_t tid;
	struct tidlist *next;
} tidlist_t;

volatile sig_atomic_t sigint_flag = 1;

/*Gestore del segnale SIGINT*/
static void sigintHandler(int signum)
{
	sigint_flag = 0;
}

/*=====================================
 * ---------THREAD WORKER--------------
 *=====================================
 */

static void *worker(void *arg)
{

	uint64_t msgrcv_skt;			 /*messaggio ricevuto (8 byte)*/
	uintmax_t time_of_arrival; /*tempo di arrivo del messaggio*/

	uint32_t stima;			 /*valore stimato del secret*/
	uint32_t *intervals; /*array contenente gli intervalli tra i tempi di arrivo*/

	int n_msgrcv;		 /*numero dei messaggi ricevuti*/
	int n_intervals; /*numero degli intervalli*/
	int bytes;			 /*numero di byte letti dalla receiveMessageOnSocket_64*/
	int size_msgsnd; /*dimensione buffer msg snd*/
	int i;					 /*variabile ausiliaria*/

	arrives_list_t *arriveslist;		/*testa alla lista degli arrivi*/
	arrives_list_t *newarrive_node; /*puntatore al nuovo elmento della lista*/

	struct timeval tv;
	message_t msgsnd;
	args_t *args;

	/*cast degli argomenti*/
	args = (args_t *)arg;

	/*====================================================
	 * CICLO DI RICEZIONE DEI MESSAGGI DAI CLIENT
	 *====================================================
	 */

	/*inizializzo le variabili di interesse prima del ciclo*/
	arriveslist = NULL;
	n_msgrcv = 0;

	/*inizio ciclo*/
	while ((bytes = receiveMessageOnSocket_64(args->fd_acc, &msgrcv_skt))) /*--->se uguale a 0 allora EOF ed esco*/
	{
		if (bytes > 0) /*---> se maggiore di 0 il messaggio è arrivato*/
		{

			/* salvo il tempo di arrivo */
			memset(&tv, 0, sizeof(struct timeval));
			gettimeofday(&tv, NULL);

			/*inserimento in testa nella lista degli arrivi*/
			if ((newarrive_node = malloc(sizeof(arrives_list_t))) == NULL)
			{
				perror("malloc");
				pthread_exit((void *)-1);
			}
			newarrive_node->next = arriveslist;
			arriveslist = newarrive_node;
			arriveslist->tv.tv_sec = tv.tv_sec;
			arriveslist->tv.tv_usec = tv.tv_usec;

			/*incremento il numero dei messaggi arrivati*/
			n_msgrcv++;

			/*stampo il tempo di arrivo*/
			time_of_arrival = ((uintmax_t)(tv.tv_sec * 1000000)) + ((uintmax_t)tv.tv_usec);
			printf("SERVER %d INCOMING FROM %" PRIx64 " @ %" PRIuMAX "\n", args->id, msgrcv_skt, time_of_arrival);
			fflush(NULL);
		}
		if (bytes < 0 && errno != EINTR) /*---> se minore di 0 sono in errore e termino il worker*/
		{
			perror("ReceiveMessageOnSocket_64");
			pthread_exit((void *)EXIT_FAILURE);
		}
	}

	/*chiudo la socket passata al worker */
	if (closeSocket(args->fd_acc) == -1)
	{
		perror("closeSocket");
		pthread_exit((void *)EXIT_FAILURE);
	}

	/* ====================================
	 *  CALCOLO STIMA
	 * ====================================
	 */

	/*se il numero di messaggi è 1 o meno allora non posso calcolare la stima e non invio nulla al supervisor*/
	if (n_msgrcv == 1)
	{

		fprintf(stderr, "SERVER %d CLOSING %" PRIx64 " CAN NOT ESTIMATE \n", args->id, msgrcv_skt);
		pthread_exit((void *)EXIT_FAILURE);
	}
	if (n_msgrcv < 1)
	{
		fprintf(stderr, "SERVER %d CLOSING unknown NO MESSAGE ARRIVED\n", args->id);
		pthread_exit((void *)EXIT_FAILURE);
	}

	/*alloco l'array degli intervalli */
	n_intervals = n_msgrcv - 1;
	if ((intervals = malloc(n_intervals * (sizeof(uint32_t)))) == NULL)
	{
		perror("malloc");
		pthread_exit((void *)EXIT_FAILURE);
	}

	/*calcolo gli intervalli*/
	calculateIntervals(arriveslist, intervals);

	/*calcolo l'MCD tra i numeri all'interno dell'array delle differenze*/
	stima = intervals[0];
	for (i = 1; i < n_intervals; i++)
		stima = euclide_MCD(stima, intervals[i]);

	/*se l'MCD calcolato è < 25 è molto probabile che uno dei valori nell'array degli intervalli contenga
		un errore causato dall'overhead del context switch. In tal caso si prende come stima il minimo  va-
		lore dell'array
	*/
	if (stima < 25)
	{
		stima = intervals[0];
		for (i = 1; i < n_intervals; i++)
		{

			if (stima > intervals[i])
			{
				stima = intervals[i];
			}
		}
	}

	/* ====================================
	 *  INVIO DELLA STIMA AL SUPERVISOR
	 * ====================================
	 */

	/*alloco il buffer di msgsnd secondo la dimensione desiderata*/
	size_msgsnd = printf("SERVER %d CLOSING %" PRIx64 " ESTIMATE %" PRIu32 "\n", args->id, msgrcv_skt, stima) + 1;
	if ((msgsnd.buffer = malloc(sizeof(char) * size_msgsnd)) == NULL)
	{
		perror("From worker->malloc");
		pthread_exit((void *)EXIT_FAILURE);
	}

	/*inizializzo la struttura*/
	msgsnd.length = size_msgsnd;
	sprintf(msgsnd.buffer, "SERVER %d CLOSING %" PRIx64 " ESTIMATE %" PRIu32 "\n", args->id, msgrcv_skt, stima);

	/*invio del messaggio*/
	if (sendMessageOnPipe(args->scrittore, &msgsnd) == -1)
	{
		perror("From worker-->sendMessageOnPipe");
		pthread_exit((void *)EXIT_FAILURE);
	}

	pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int id;				 /*numero identificatore del server*/
	int fd_skt;		 /*file descriptor della socket	  */
	int fd_acc;		 /*file descriptor di ritorno dalla accept*/
	int scrittore; /*file descriptor di scrittura sulla pipe*/
	int lettore;	 /*file descriptor di lettura sulla pipe*/

	char socket_name[UNIX_PATH_MAX]; /*buffer contenente il nome della socket*/

	tidlist_t *tidlist, *newtid_node; /*thread identifier*/
	args_t *args;											/*struttura per il passaggio degli argomenti al thread*/

	/* =================================
	 *  GESTIONE SEGNALI
	 * =================================
	 */

	sigset_t set;
	struct sigaction sa;

	/* maschero tutti i segnali */
	if (sigfillset(&set) == -1)
	{
		perror("sigfillset");
		exit(EXIT_FAILURE);
	}
	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
	{
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}

	/*installo i gestori*/
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigintHandler;
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	/*tolgo la maschera */
	if (sigemptyset(&set) == -1)
	{
		perror("sigemptyset");
		exit(EXIT_FAILURE);
	}
	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
	{
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}

	/* =================================
	 *  CHECK ARGOMENTI
	 * =================================
	 */

	if (argc != 4)
	{
		errno = EINVAL;
		perror("Numero argomenti");
		exit(EXIT_FAILURE);
	}
	if (sscanf(argv[1], "%d", &id) == -1)
	{
		errno = EINVAL;
		perror("Tipo primo argomento");
		exit(EXIT_FAILURE);
	}
	if (sscanf(argv[2], "%d", &lettore) == -1)
	{
		errno = EINVAL;
		perror("Tipo secondo argomento");
		exit(EXIT_FAILURE);
	}
	if (sscanf(argv[3], "%d", &scrittore) == -1)
	{
		errno = EINVAL;
		perror("Tipo terzo argomento");
		exit(EXIT_FAILURE);
	}

	/* =================================
	 * 	CORPO DISPATCHER
	 * =================================
	 */

	/*il server effettua solo scritture sulla pipe*/
	if (closePipe(lettore) == -1)
	{
		perror("closePipe");
		exit(EXIT_FAILURE);
	}

	/*stampa di avvio*/
	printf("SERVER %d ACTIVE\n", id);

	/*costruisco il nome della socket*/
	sprintf(socket_name, SOCK_NAME, id);

	/*creo la socket lato server*/
	fd_skt = createServerChannel(socket_name);
	ec_neg1(fd_skt, "CreateServerChannel");

	/*ciclo dispatcher delle connessioni entranti*/
	tidlist = NULL;
	while (sigint_flag == 1)
	{
		/*accept bloccante*/
		fd_acc = acceptConnection(fd_skt);

		/*accept interrotta da un segnale*/
		if (fd_acc == -1 && errno == EINTR)
		{
			/*possibile sched_yield()*/
			continue;
		}

		/*errore nella accept*/
		if (fd_acc == -1 && errno != EINTR)
		{
			exit(EXIT_FAILURE);
		}

		fprintf(stdout, "SERVER %d CONNECT FROM CLIENT\n", id);

		/*alloco e setto la struttura contenente gli argomenti da passare al worker*/
		if ((args = malloc(sizeof(args_t))) == NULL)
		{
			exit(EXIT_FAILURE);
		}
		args->id = id;
		args->fd_acc = fd_acc;
		args->scrittore = scrittore;

		/*alloco nuovo nodo per tidlist*/
		newtid_node = malloc(sizeof(tidlist_t));
		newtid_node->next = tidlist;
		tidlist = newtid_node;

		/*creazione del thread worker*/
		pthread_create(&(tidlist->tid), NULL, &worker, (void *)args);
	}

	/* =================================
	 *  CLEAN UP
	 * =================================
	 */

	errno = 0;
	while (tidlist != NULL)
	{
		pthread_join(tidlist->tid, NULL);
		tidlist = tidlist->next;
	}

	closeSocket(fd_skt);
	unlink(socket_name);
	closePipe(scrittore);

	exit(EXIT_SUCCESS);
}
