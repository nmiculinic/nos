#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef enum {READY=1, SHOOT, HIT, MISS, DEAD, FINALIZE} message_type;

typedef struct {
    long pid;
    message_type mtype;
    int r;
    int c;
} message_t;

char S[10][10];

void read_field(){
  for (size_t i = 0; i < 4; i++) {
    scanf(" %s", S[i]);
  }
}

int pid;
int msqid;
const key_t key = 12345;

void send(message_t* m){
  m -> pid = (pid + 1) % 2 + 1;
  if (msgsnd(msqid, &m, sizeof(message_t), 0) == -1) {
    perror("msgsnd+");
  }
}

void rcv(message_t* m){
  if (msgrcv(msqid, &m, sizeof(m)-sizeof(long), pid, 0) == -1) {
      perror("msgrcv");
      exit(1);
  }
}

bool send_shoot() {
  printf("ispali na polje: ");
  message_t m;
  m.mtype = SHOOT;
  scanf("%d %d\n", &m.r, &m.c);
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

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("invalid no arguments, quitting\n");
    exit(1);
  }

	if (strcmp(argv[1], "p1") == 0) {
		printf("Player1 %s\n", argv[1]);
    pid = 1;
	} else {
    printf("Player2 %s\n", argv[1]);
    pid = 2;
  }

  printf("Creating message queues\n");
  if ((msqid = msgget(key, 0600 | IPC_CREAT)) == -1) {
      perror("msgget");
      exit(1);
  }

  read_field();

  if (pid == 1) {
    p1();
  } else {
    p2();
  }
  return 0;
}
