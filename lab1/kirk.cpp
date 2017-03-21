/*
** kirk.c -- writes to a message queue
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct my_msgbuf {
    long dddd;
    char mtext[200];
    int test;
};

int main(void)
{
    struct my_msgbuf buf;
    int msqid;
    key_t key;
    char text[]="Kirk: We are attacked. Spock, send reinforcement.";

    key = 12345;

    if ((msqid = msgget(key, 0600 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }

    memcpy(buf.mtext, text, strlen(text)+1);
    buf.dddd = 10;

    if (msgsnd(msqid, (struct msgbuf *)&buf, strlen(text)+1, 0) == -1)
            perror("msgsnd");

    printf("Kirk: Reported attacks to Spock, he will send help!\n");

    return 0;
}
