/** \file comsock.c
		\author Yuri Caprini
		\brief Implementazione delle funzioni definite nell'header file comsock.h

	 Si dichiara che il contenuto di questo file e' in ogni sua parte opera
	 originale dell' autore.  */

#include "comsock.h"
#include "endian.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/**
 * \def CHAR_LENGTH
 *
 * Dimensione in caratteri del campo dimensioni.
 * Necessaria per effettuare la serializzazione del messaggio e la sua ricezione
 *
 * \internal Necessaria per effetuare la serializzazione e la deserializzazione
 *
 * \see sendMessage
 * \see receiveMessage
 */
#define CHAR_LENGTH 4

/**
 * \def BUFF_SIZE
 *
 * Dimensione del campo buffer del messaggio.
 * Necessaria per effettuare la serializzazione del messaggio e la sua ricezione
 *
 * \internal Necessaria per effetuare la serializzazione e la deserializzazione
 *
 * \see sendMessage
 * \see receiveMessage
 */
#define BUFF_SIZE 512

int createServerChannel(char *path)
{

	int sfd;
	struct sockaddr_un sa;

	/*check degli argomenti*/
	if (path == NULL || strlen(path) == 0)
	{
		errno = EINVAL;
		return -1;
	}

	if (strlen(path) > UNIX_PATH_MAX)
	{
		errno = E2BIG;
		return -1;
	}

	/*creazione socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	/*struttura socket inizializzata*/
	memset(&sa, 0, sizeof(struct sockaddr_un));
	strcpy(sa.sun_path, path);
	sa.sun_family = AF_UNIX;

	if (bind(sfd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
	{
		closeSocket(sfd);
		return -1;
	}

	if (listen(sfd, SOMAXCONN) == -1)
	{
		closeSocket(sfd);
		return -1;
	}

	return sfd;
}

int closeSocket(int s)
{
	int result;

	/* nel caso in cui sia interrotta la rimando in esecuzione */
	while ((result = close(s)))
	{
		if (result == -1 && errno == EINTR)
		{
			continue;
		}
		break;
	}

	return result;
}

int acceptConnection(int s)
{

	int acc_fd;

	if ((acc_fd = accept(s, NULL, 0)) == -1)
		return -1;
	else
		return acc_fd;
}

int receiveMessageOnSocket_64(int sc, uint64_t *msg)
{

	int bytes;
	int count;

	/*Check argomenti*/
	if (msg == NULL || sc < 0)
	{
		errno = EINVAL;
		return -1;
	}

	/*Lettura id dalla socket*/
	count = sizeof(uint64_t);
	while (1)
	{
		bytes = read(sc, msg, count);

		/*se la read legge meno byte di quelli previsti
			si rieffettua la lettura dei byte mancanti*/
		if (0 < bytes && bytes < count)
		{
			count = count - bytes;
			msg = msg + bytes;
			continue;
		}

		break;
	}

	/*conversione in hostbyteorder*/
	if (bytes > 0)
	{
		*msg = ntoh64(*msg);
	}

	return bytes;
}

int sendMessageOnSocket_64(int sc, uint64_t msg)
{

	unsigned int bytes;
	uint64_t *msgaddr;
	int count;

	/*Check argomenti*/
	if (sc < 0)
	{
		errno = EINVAL;
		return -1;
	}

	/*conversione in networkbyteorder */
	msg = hton64(msg);
	msgaddr = &msg;

	/*scrittura sulla socket con controllo errori*/
	count = sizeof(uint64_t);
	while ((bytes = write(sc, msgaddr, count)))
	{
		/*se la write scrive meno byte di quelli previsti
			si rieffettua la scrittura dei byte mancanti*/
		if (0 < bytes && bytes < count)
		{
			count = count - bytes;
			msgaddr = msgaddr + bytes;
			continue;
		}

		/*nel caso di interruzione prima della scrittura, rieffettuo la chiamata*/
		if (bytes < 0 && errno == EINTR)
		{
			continue;
		}

		break;
	}

	return bytes;
}

int openConnection(char *path)
{

	int i = 0, conn_fd;
	struct sockaddr_un sa;

	if (path == NULL || strlen(path) == 0)
		return -1;

	if (strlen(path) > UNIX_PATH_MAX)
	{
		errno = E2BIG;
		return -1;
	}

	memset(&sa, 0, sizeof(struct sockaddr_un));

	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);

	if ((conn_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	while (i < NTRIALCONN)
	{
		if (connect(conn_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		{
			sleep(1);
			i++;
		}
		else
			break;
	}

	if (i == NTRIALCONN)
		return -1;
	else
		return conn_fd;
}
