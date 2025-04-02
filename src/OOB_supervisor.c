#include "compipe.h"
#include "genlist.h"

#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>

#define SERV_ELF_PATH "./OOB_server"
#define AUX_BUF_LEN 11

/* =================================================================================================
 *  																		VARIABILI GLOBALI
 * =================================================================================================
 */

/* I file descriptor della pipe in lettura e scrittura */
int pfd[2] = {-1, -1};

/**
 * Flag che se settato a 1 indica l'avvenuto arrivo di un segnale di SIGALRM.
 * In caso contrario è settato a 0.
 */
volatile sig_atomic_t sigalrm_flag = 1;

/**
 * \var print_flag
 * Flag che se settato a 1, indica il permesso di
 * stampa della tabella delle stime su stderr.
 * In caso contrario è settato a 0.
 * Quest'ultimo è anche il suo valore di default.
 */
volatile sig_atomic_t print_flag = 0;

/**
 * \var doublesigint_flag
 * Flag che se settato a 1, indica l'avvenuto arrivo consecutivo
 * di due segnali di SIGINT nell'arco di 1 secondo.
 * In caso contrario è settato a 0.
 * Quest'ultimo è anche il suo valore di default.
 */
volatile sig_atomic_t doublesigint_flag = 0;

/* ===========================================
 *  PROCEDURE, FUNZIONI E STRUTTURE INTERNE
 * ===========================================
 */

/**
 * Struttura che contiene la stima e
 * le informazioni ad essa associate
 */
typedef struct est_info
{
	uint64_t id_cli; /**< l'id del client */
	uint32_t est;		 /**< il valore della stima */
	int n;					 /**< il numero di server che ha fornito la stima
									per questo client */
} est_info_t;

/** Procedura di stampa della stima e delle info associate su stderr.
 *
 *	Viene passata alla procedura printList(), per il casting e la successiva
 *	stampa del campo void * elem di ogni nodo della lista, secondo il tipo e
 *	formato voluto.
 *	In questo caso il casting è effettuato da (void*)est_el-->(est_info_t *)est_el.
 *
 *  \param 		est_el è il campo elem di ogni nodo della lista generica
 *  \retval		void
 */
void printEstOnStderr(void *est_el)
{
	fprintf(stderr, "SUPERVISOR ESTIMATE %" PRIu32 " FOR %" PRIx64 " BASED ON %d\n", ((est_info_t *)est_el)->est, ((est_info_t *)est_el)->id_cli, ((est_info_t *)est_el)->n);
}

/** Procedura di stampa della stima e delle info associate su stdout.
 *
 *	Viene passata alla procedura printList(), per il casting e la successiva
 *	stampa del campo void * elem di ogni nodo della lista, secondo il tipo e
 *	formato voluto.
 *	In questo caso il casting è effettuato da (void*)est_el-->(est_info_t *)est_el.
 *
 *  \param 		est_el    è il campo elem di ogni nodo della lista generica
 *	\retval		void
 */
void printEstOnStdout(void *est_el)
{
	fprintf(stdout, "SUPERVISOR ESTIMATE %" PRIu32 " FOR %" PRIx64 " BASED ON %d\n", ((est_info_t *)est_el)->est, ((est_info_t *)est_el)->id_cli, ((est_info_t *)est_el)->n);
}

/** Funzione di comparazione di due elementi est_info_t in base al loro campo id_cli.
 *
 *	Presi due elementi est_el1, est_el2  di tipo est_info_t,li confronta in base
 *	al loro campo id_cli e restituisce un intero maggiore,minore o uguale di zero,
 *  se est_el1 è rispettivamente maggiore,minore o uguale di est_el2.
 *
 *	/param 	est_el1   primo elemento est_info_t
 *	/param 	est_el2	  secondo elemento est_info_t
 *
 *	/retval 	  1   se est_el1.id_cli > est_el2.id_cli
 *  /retval 	 -1   se est_el1.id_cli < est_el2.id_cli
 *  /retval 	  0   se est_el1.id_cli == est_el2.id_cli
 */
int cmpEstEl_idcli(est_info_t est_el1, est_info_t est_el2)
{
	int res;

	if (est_el1.id_cli > est_el2.id_cli)
		res = 1;

	if (est_el1.id_cli < est_el2.id_cli)
		res = -1;

	if (est_el1.id_cli == est_el2.id_cli)
		res = 0;

	return res;
}

/** Funzione di comparazione di due elementi est_info_t in base al loro campo est.
 *
 *	Presi due elementi est_el1, est_el2  di tipo est_info_t,li confronta in base
 *	al loro campo est e restituisce un intero maggiore,minore o uguale di zero,
 *  se est_el1 è rispettivamente maggiore,minore o uguale di est_el2.
 *
 *	/param 	est_el1   primo elemento est_info_t
 *	/param 	est_el2	  secondo elemento est_info_t
 *
 *	/retval 	  1   se est_el1.est > est_el2.est
 *  /retval 	 -1   se est_el1.est < est_el2.est
 *  /retval 	  0   se est_el1.est == est_el2.est
 */
int cmpEstEl_est(est_info_t est_el1, est_info_t est_el2)
{
	int res;

	if (est_el1.est > est_el2.est)
		res = 1;

	if (est_el1.est < est_el2.est)
		res = -1;

	if (est_el1.est == est_el2.est)
		res = 0;

	return res;
}

/* Funzione di aggiornamento della lista delle stime correnti effettuate dal supervisor.
 *
 * Data la lista delle stime correnti estlist e un nuovo elemento new_est_el,la funzione
 * controlla l'esistenza nella lista di un elemento con campo id_cli, identico a quello
 * di new_est_el.
 *
 * Nel caso in cui esista, la funzione incrementa il campo n dell'elemento corrente
 * in lista e procede a comparare il suo campo est con quello di new_est_el.
 * 	- Se risulta maggiore di quello di new_est_el, allora il campo stima dell'elemento
 *	  corrente viene aggiornato con quello di new_est_list.
 *	- Se risulta minore o uguale a quello di new_est_el allora, semplicemente new_est_el
 * 	  viene scartato.
 *
 * Nel caso in cui invece non esista, effettuo l'inserimento di new_est_el in lista.
 *
 * /param	estlist 		la lista delle stime correnti
 * /param   new_est_el		nuovo elemento
 * /result	new_estlist		la lista aggiornata
 * /result	NULL			in caso di errore
 */
genlist_t updateEstList(genlist_t estlist, est_info_t *new_est_el)
{
	genlist_t aux;
	est_info_t *curr_est_el;
	int found = 0;
	if (new_est_el == NULL)
	{
		return NULL;
	}
	/* se la lista è vuota, inserisco il nuovo elemento in lista*/
	if (estlist == NULL)
	{
		if ((estlist = insertList(estlist, new_est_el)) == NULL)
		{
			return NULL;
		}
	}
	/* altrimenti*/
	else
	{
		aux = estlist;

		/*cerco nella lista un elemento con lo stesso id_cli di new_est_el*/
		while (aux != NULL)
		{
			curr_est_el = ((est_info_t *)aux->elem);

			/*se lo trovo*/
			if (cmpEstEl_idcli(*curr_est_el, *new_est_el) == 0)
			{
				found = 1;
				/* controllo se il campo est dell'elemento corrente
					 in lista è maggiore di quello di new_est.*/
				if (cmpEstEl_est(*curr_est_el, *new_est_el) > 0)
				{
					/* in tal caso aggiorno il campo est dell'elemento
					 corrente con quello di new_est_el*/
					curr_est_el->est = new_est_el->est;
				}
				/* in ogni caso incremento il campo n dell'elemento corrente */
				curr_est_el->n = curr_est_el->n + 1;
				free(new_est_el);
				break;
			}
			aux = aux->next;
		}

		/*se non ho trovato nessun elemento con id_cli uguale a
		 *new_est_el,allora inserisco quest'ultimo in lista*/
		if (found == 0)
		{
			if ((estlist = insertList(estlist, new_est_el)) == NULL)
			{
				return NULL;
			}
		}
	}

	return estlist;
}

/** Procedura di cleanup.
 *
 *  Viene registrata all'interno della atexit() all'inizio dell'esecuzione del supervisor.
 *	Chiude la pipe in lettura e scrittura appena prima del termine del programma.
 */
void cleanup()
{
	if (pfd[0] != -1 && pfd[1] != -1)
	{
		if (closePipe(pfd[0]) == -1 && errno != EBADF)
			perror("closePipe");
		if (closePipe(pfd[1]) == -1 && errno != EBADF)
			perror("closePipe");
	}
}

/*Gestore del segnale SIGALRM.
 *
 * Ogni volta che viene eseguito, significa che è passato un secondo dall'ultimo SIGINT.
 * Viene quindi abilitata la stampa delle stime su stderr settando print_flag a 1.
 * Anche il flag di avvenuto sigalarm_flag viene settato a 1, per permettere al successivo SIGINT
 * di rieseguire alarm(1).
 */
static void sigalrmHandler(int signum)
{
	sigalrm_flag = 1;
}

/*Gestore del segnale SIGINT.
 *
 * Controlla sigalarm_flag:
 * -se è a 1 allora un secondo è già passato e sia il flag che il timer devono essere riazzerati.
 *
 * -se è a 0 invece significa che ancora non è passato un secondo dall'ultimo SIGINT e va dunque
 *  abilitata la chiusura del supervisor, dei server e la stampa delle stime su stdout settando
 *  doublesigint_flag a 1
 */
static void sigintHandler(int signum)
{
	if (sigalrm_flag == 0)
	{
		doublesigint_flag = 1;
	}
	else
	{
		sigalrm_flag = 0;
		print_flag = 1;
		alarm(1);
	}
}

/* =================================================================================================
 * 																					SUPERVISOR
 * =================================================================================================
 *
 * - Usage: ./OOB_supervisor k
 * - Crea k server con i quali resta in comunicazione tramite pipe
 * - Attende le stime da parte dei server e effettua le proprie stime a partire da quest'ultime
 */
int main(int argc, char *argv[])
{
	/*numero dei server da creare*/
	int k;
	/*indice ausiliario*/
	int i;
	/*numero dei byte letti dalla receiveMessageOnPipe*/
	int bytes;
	/*variabile utilizzata per la stampa dell'id del server*/
	int id_serv;
	/*array contenente i pid dei server*/
	int *pids;
	/*primo argomento da passare al server*/
	char arg1[AUX_BUF_LEN];
	/*secondo argomento da passare al server*/
	char arg2[AUX_BUF_LEN];
	/*terzo argomento da passare al server*/
	char arg3[AUX_BUF_LEN];
	/*struttura del msg in ricezione */
	message_t msgrcv;
	/*lista contenente i messaggi nella struttura msg_parsed*/
	genlist_t estlist;
	/*nuovo elemento della lista delle stime*/
	est_info_t *new_est_el;

	/* ===============================================================================================
	 *  																	GESTIONE SEGNALI
	 * ===============================================================================================
	 */

	sigset_t set;
	struct sigaction sa;

	/*registro la funzione di cleanup*/
	if (atexit(cleanup) != 0)
	{
		perror("atexit");
		exit(EXIT_FAILURE);
	}

	memset(&set, 0, sizeof(set));
	memset(&sa, 0, sizeof(sa));

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
	sa.sa_handler = sigintHandler;
	sa.sa_mask = set;
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = sigalrmHandler;
	if (sigaction(SIGALRM, &sa, NULL) == -1)
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

	/* ===============================================================================================
	 *  																	CHECK ARGOMENTI
	 * ===============================================================================================
	 */

	if (argc != 2)
	{
		errno = EINVAL;
		perror("Numero di argomenti");
		exit(EXIT_FAILURE);
	}
	if (sscanf(argv[1], "%d", &k) == -1)
	{
		errno = EINVAL;
		perror("Primo argomento");
		exit(EXIT_FAILURE);
	}

	/* ===============================================================================================
	 *  																	CREAZIONE DEI SERVER
	 * ===============================================================================================
	 */

	/*messaggio di avvio che comunica il numero di server avviati (k)*/
	printf("SUPERVISOR STARTING %d\n", k);

	if (createPipe(pfd) == -1)
	{
		perror("createPipe");
		exit(EXIT_FAILURE);
	}

	/*setto gli argomenti da passare al server*/
	snprintf(arg2, AUX_BUF_LEN, "%d", pfd[0]);
	snprintf(arg3, AUX_BUF_LEN, "%d", pfd[1]);

	/*preparo buffer per memorizzare i pid dei server*/
	if ((pids = malloc(sizeof(int) * k)) == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (i = 1; i <= k; i++)
	{
		snprintf(arg1, AUX_BUF_LEN, "%d", i);

		/*un processo figlio per ogni server: è un replica identica del padre*/
		if ((pids[i - 1] = fork()) == -1)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		/*nel figlio la fork avrà restituito 0, sostituisco programma originale con quello del server*/
		if (pids[i - 1] == 0)
		{
			setpgrp(); // necessario per evitare la propagazione di SIGINT ai figli con CTRL+C
			execl(SERV_ELF_PATH, "OOB_server", arg1, arg2, arg3, NULL);
		}
	}

	/*chiudo pipe in scrittura*/
	if (closePipe(pfd[1]) == -1)
	{
		perror("closePipe");
		exit(EXIT_FAILURE);
	}

	/* ===============================================================================================
	 *  																	RICEZIONE MESSSAGGI DAI SERVER
	 * ===============================================================================================
	 */

	estlist = NULL;
	while (TRUE)
	{
		bytes = receiveMessageOnPipe(pfd[0], &msgrcv);

		/*lato scrittura chiuso*/
		if (bytes == 0)
		{
			break;
		}

		/*altrimenti ricevuto qualcosa*/
		if (bytes > 0)
		{
			/*parsing del messaggio*/
			if ((new_est_el = malloc(sizeof(est_info_t))) == NULL)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}
			sscanf(msgrcv.buffer, "SERVER %d CLOSING %" PRIx64 " ESTIMATE %" PRIu32 "\n",
						 &id_serv, &(new_est_el->id_cli), &(new_est_el->est));
			new_est_el->n = 1;

			/*stampa messaggio di arrivo*/
			printf("SUPERVISOR ESTIMATE %" PRIu32 " FOR %" PRIx64 " FROM %d\n",
						 new_est_el->est, new_est_el->id_cli, id_serv);

			/*aggiornamento della lista delle stime correnti*/
			if ((estlist = updateEstList(estlist, new_est_el)) == NULL)
			{
				perror("updateEstList");
				exit(EXIT_FAILURE);
			}
		}

		/*errore nella receiveMessageOnPipe*/
		if (bytes < 0 && errno != EINTR)
		{
			perror("receiveMessageOnPipe");
			exit(EXIT_FAILURE);
		}

		/*ricevuto singolo SIGINT: stampo le stime correnti */
		if (print_flag == 1)
		{
			printList(estlist, printEstOnStderr);
			fprintf(stderr, "\n");
			print_flag = 0;
		}

		/*ricevuto doppio SIGINT: termino tutti i server*/
		if (doublesigint_flag == 1)
		{
			for (i = 0; i < k; i += 1)
			{
				kill(pids[i], SIGINT);
			}
			doublesigint_flag = 0;
		}
	}

	/*effettuo la stampa su stdout della lista delle stime*/
	printList(estlist, printEstOnStdout);
	printf("SUPERVISOR EXITING\n");

	exit(EXIT_SUCCESS);
}
