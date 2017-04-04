#include <algorithm>
#include <cstdlib>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const int NUM_PROC = 5;
// https://www.wikiwand.com/en/Lamport_timestamps
// https://www.wikiwand.com/en/Lamport%27s_distributed_mutual_exclusion_algorithm

unsigned int time = 0;
using namespace std;

typedef enum { TIMEOUT = 0, REQUEST, GRANT, RELEASE } message_type;
const char *message_type_names[] = {"TIMEOUT", "REQUEST", "GRANT", "RELEASE"};

struct message_t {
  unsigned int time_stamp;
  int origin;
  message_type type;
};

// from, to, FDs
int fd[NUM_PROC][NUM_PROC][2];
int id;

void send(message_type type) {
  time = time + 1;
  message_t message;
  message.time_stamp = time;
  message.type = type;
  message.origin = id;

  for (int i = 0; i < NUM_PROC; ++i) {
    if (i == id)
      continue;
    printf("[%d] SND %d -> %d %s\n", time, id, i,
           message_type_names[message.type]);
    write(fd[id][i][1], &message, sizeof(message_t));

    message_t message;
    int bread = read(fd, &message, sizeof(message));
  }
}

void rcv_fd(int fd) {
  message_t message;
  int bread = read(fd, &message, sizeof(message));
  if (bread != sizeof(message_t)) {
    printf("ERROR!!!_\n");
  }
  time = max(message.time_stamp, time) + 1;
  printf("[%d] RCV %d -> %d %s\n", time, message.origin, id,
         message_type_names[message.type]);

  /**
    Important processing
  */
  switch (message.type) {
  case REQUEST:
  case GRANT:
  case RELEASE:
  default:
    break;
  }
}

void rcv(int wait_time) {
  fd_set rfds;
  FD_ZERO(&rfds);
  for (int i = 0; i < NUM_PROC; ++i) {
    if (i == id)
      continue;
    FD_SET(fd[i][id][0], &rfds);
  }

  struct timeval wt;
  wt.tv_sec = wait_time;
  wt.tv_usec = 0;
  int retval = select(1, &rfds, NULL, NULL, &wt);

  if (retval == -1)
    perror("select()");
  else if (retval) {
    printf("NASAO nes %d\n", retval);
    for (int i = 0; i < NUM_PROC; ++i) {
      if (i == id)
        continue;
      if (FD_ISSET(fd[i][id][0], &rfds))
        rcv_fd(fd[i][id][0]);
    }
  }
}

void start_child() {
  for (int i = 0; i < NUM_PROC; ++i) {
    close(fd[id][i][0]);
    close(fd[i][id][1]);
  }

  for (int i = 0; i < 1; ++i) {
    send(REQUEST);
    rcv(1);
  }
  exit(0);
}

int main(void) {

  for (size_t i = 0; i < NUM_PROC; i++) {
    for (size_t j = 0; j < NUM_PROC; j++) {
      if (pipe(fd[i][j]) == -1)
        printf("ERROR u otvaranju cjevovoda\n", );
    }
  }

  for (size_t i = 0; i < NUM_PROC; i++) {
    switch (fork()) {
    case -1:
      exit(1);
    case 0:
      id = i;
      start_child();
    default:
      break;
    }
  }
  wait(NULL);
  exit(0);
}
