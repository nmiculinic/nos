#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

typedef enum {READY=1, SHOOT, HIT, MISS, DEAD, FINALIZE} message_type;

struct message_t {
    long pid;
    message_type mtype;
    int r;
    int c;
};

char S[10][10];

void read_field(){
  for (size_t i = 0; i < 4; i++) {
    scanf(" %s", S[i]);
  }
}

int pid;
int msqid;
const key_t key = 12345;

const size_t msg_sz = sizeof(message_t) - sizeof(long);

void send(message_t* m){
  m->pid = pid^3;
  // printf("%d Saljem poruku pid %d\n", pid, m->pid);
  if (msgsnd(msqid, m, msg_sz, 0) == -1) {
    perror("msgsnd+");
  }
}

void rcv(message_t* m){
  // printf("Cekam poruku %d\n", pid);
  int lenn = msgrcv(msqid, m, msg_sz, pid, 0);
  if (lenn == -1) {
      perror("msgrcv");
      exit(1);
  }
  // printf("%d\n", lenn);
  // printf("Primio poruku %ld\n", m->pid);
}

bool send_shoot() {
  printf("ispali na polje: ");
  message_t m;
  m.mtype = SHOOT;
  scanf(" %d %d", &m.r, &m.c);
  send(&m);
  rcv(&m);

  switch (m.mtype) {
    case HIT:
      printf("Pogodak\n");
      return true;
    case MISS:
      printf("Promasaj\n");
      return true;
    case DEAD:
      printf("Pogodak. Pobjeda\n");
      return false;
    default:
      printf("wtf %d\n", m.mtype);
      exit(1);
  }
}

bool am_i_dead() {
  for (size_t i = 0; i < 4; i++) {
    if (strcmp(S[i], "----") != 0)
      return false;
  }
  return true;
}

bool rcv_shoot() {
  message_t m;
  rcv(&m);

  printf("pucano na polje: %d %d\n", m.r, m.c);
  if (S[m.r][m.c] == 'o') {
    printf("Pogodak\n");
    S[m.r][m.c] = '-';
    if (am_i_dead())
      m.mtype = DEAD;
    else
      m.mtype = HIT;
  } else {
    m.mtype = MISS;
  }
  send(&m);
  return m.mtype != DEAD;
}

void verify_read(){
  message_t m;
  m.mtype = READY;
  send(&m);
  rcv(&m);
  if (m.mtype != READY) {
    printf("ERROR %d\n", m.mtype);
  }
}

void p1(){
  for (;;) {
    if(!send_shoot()) break;
    if(!rcv_shoot()) break;
  }
  msgctl(msqid, IPC_RMID, NULL);
}

void p2(){
  for (;;) {
    if(!rcv_shoot()) break;
    if(!send_shoot()) break;
  }
}

void retreat(int failure)
{
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("invalid no arguments, quitting\n");
    exit(1);
  }

	if (strcmp(argv[1], "p1") == 0) {
		printf("Player1 %s\n", argv[1]);
    pid = 1;
	} else if (strcmp(argv[1], "p2") == 0) {
    printf("Player2 %s\n", argv[1]);
    pid = 2;
  } else {
    printf("Unknown player!!!\n");
    exit(1);
  }

  printf("PID %d\n", pid);
  printf("Creating message queues; msg_sz %lu\n", msg_sz);
  if ((msqid = msgget(key, 0600 | IPC_CREAT)) == -1) {
      perror("msgget");
      exit(1);
  }
  sigset(SIGINT, retreat);
  printf("MSGID %d\n", msqid);

  read_field();

  if (pid == 1) {
    p1();
  } else {
    p2();
  }
  return 0;
}
