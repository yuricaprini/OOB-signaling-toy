#include "comsock.h"
#include "macros.h"
#include "internals.h"
#include "endian.h"

#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define SECRET_UPPERBOUND 3000

int main(int argc, char *argv[])
{
	/*numero di server totali in esecuzione*/
	int k;
	/*numero di server a cui il client si connette*/
	int p;
	/*numero totale di messaggi inviati dal client (ciascun messaggio è inviato a uno dei p server)*/
	int w;
	/*il pool da cui estrarre randomicamente gli id (distinti) dei server a cui connettersi*/
	int *srv_id_pool;
	/*dimensione del pool */
	int size_srv_id_pool;
	/*l'id del server estratto dal pool, a cui il client dovrà connettersi*/
	int rand_srv_id;
	/*file descriptor della socket*/
	int fd_skt;
	/*buffer contenente il nome della socket*/
	char socket_name[UNIX_PATH_MAX];
	/*buffer contenente gli id dei server a cui il client è collegato*/
	int *servers_id;
	/*buffer contenente gli fd dei server a cui il client è collegato*/
	int *servers_fd;
	/*indice del server a cui inviare il messaggio corrente per i buffer servers_id e servers_fd*/
	int dest_server_index;
	/*numero segreto del client*/
	int secret;
	/*identificatore unico del client*/
	uint64_t client_id;
	/*indice ausiliario*/
	int i;

	/*
	 * ===============================================================================================
	 *  																	CHECK DEGLI ARGOMENTI
	 * ===============================================================================================
	 */

	if (argc != 4)
	{
		errno = EINVAL;
		perror("Numero argomenti");
		exit(EXIT_FAILURE);
	}

	if (sscanf(argv[1], "%d", &p) < 0)
	{
		errno = EINVAL;
		perror("Tipo primo argomento");
		exit(EXIT_FAILURE);
	}

	if (sscanf(argv[2], "%d", &k) < 0)
	{
		errno = EINVAL;
		perror("Tipo secondo argomento");
		exit(EXIT_FAILURE);
	}

	if (sscanf(argv[3], "%d", &w) < 0)
	{
		errno = EINVAL;
		perror("Tipo terzo argomento");
		exit(EXIT_FAILURE);
	}

	if (((1 <= p) && (p < k)) == 0)
	{
		errno = EINVAL;
		perror("Limiti primo argomento");
		exit(EXIT_FAILURE);
	}

	if (k < 0)
	{
		errno = EINVAL;
		perror("Limiti secondo argomento");
		exit(EXIT_FAILURE);
	}

	if (w <= (3 * p))
	{
		errno = EINVAL;
		perror("Limiti terzo argomento");
		exit(EXIT_FAILURE);
	}

	/*
	 * ===============================================================================================
	 *  																	  CORPO DEL CLIENT
	 * ===============================================================================================
	 */

	/*generazione id e secret*/
	srand(time(NULL) + getpid());
	client_id = rand_uint64();
	secret = (rand() % SECRET_UPPERBOUND) + 1;

	printf("CLIENT %" PRIx64 " SECRET %d\n", client_id, secret);

	/*inizializzazione buffer (ad indici uguali corrispondono server uguali) */
	ec_null(servers_id = malloc(p * sizeof(int)), perror("malloc"); exit(EXIT_FAILURE);)
			ec_null(servers_fd = malloc(p * sizeof(int)), perror("malloc"); exit(EXIT_FAILURE);)
					ec_null(srv_id_pool = malloc(k * sizeof(int)), perror("malloc"); exit(EXIT_FAILURE);)
							size_srv_id_pool = k;

	for (i = 0; i < size_srv_id_pool; i += 1)
	{
		srv_id_pool[i] = i + 1;
	}

	/*scelgo in modo pseudo-casuale p server distinti dai k attivi ed effettuo la connessione*/
	i = 0;
	while (i < p)
	{

		rand_srv_id = randFromPool(&size_srv_id_pool, srv_id_pool);

		if (rand_srv_id == -1)
		{
			perror("randFromPool");
			exit(EXIT_FAILURE);
		}

		/*connessione al server numero "rand_srv_id"*/
		sprintf(socket_name, "OOB-server-%d", rand_srv_id);

		fd_skt = openConnection(socket_name);
		if (fd_skt == -1)
		{
			perror("openConnection");
			exit(EXIT_FAILURE);
		}

		/*salvataggio delle coppie <id server, file descriptor associato>*/
		servers_id[i] = rand_srv_id;
		servers_fd[i] = fd_skt;

		i++;
	}

	free(srv_id_pool);

	/*ciclo di invio di w messaggi a p server scelti pseudocasualmente*/
	for (i = 0; i < w; i++)
	{
		dest_server_index = rand() % p;

		/*invio del messaggio*/
		if (sendMessageOnSocket_64(servers_fd[dest_server_index], client_id) == -1)
		{

			if (errno == EPIPE)
			{
				perror("sendMessageOnSocket_64");

				if (closeSocket(servers_fd[dest_server_index]) == -1)
				{
					perror("closeSocket");
					exit(EXIT_FAILURE);
				}
			}

			else
			{
				perror("sendMessageOnSocket");
				exit(EXIT_FAILURE);
			}
		}

		milliSleep(secret);
	}

	/*
	 * ===============================================================================================
	 *  																	  CLEAN UP
	 * ===============================================================================================
	 */

	for (i = 0; i < p; i++)
	{
		if (closeSocket(servers_fd[i]) == -1)
		{
			perror("closeSocket");
			exit(EXIT_FAILURE);
		}
	}

	/*stampo messaggio di chiusura*/
	fprintf(stdout, "CLIENT %" PRIx64 " DONE\n", client_id);

	exit(EXIT_SUCCESS);
}
