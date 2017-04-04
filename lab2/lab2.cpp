// https://www.wikiwand.com/en/Lamport_timestamps
// https://www.wikiwand.com/en/Lamport%27s_distributed_mutual_exclusion_algorithm


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

const int NUM_PROC = 2;
const int MAXN=1000;

unsigned int time = 0;
using namespace std;

typedef enum { REQUEST, GRANT, RELEASE } message_type;
const char *message_type_names[] = {"REQUEST", "GRANT", "RELEASE"};

struct message_t {
  unsigned int time_stamp;
  int origin;
  message_type type;
};

// from, to, FDs
int fd[NUM_PROC][NUM_PROC][2];
int id;

int request_queue[NUM_PROC];
bool granted[NUM_PROC];

void send_to(message_type type, int target) {
    time = time + 1;
    if (type == REQUEST) {
        request_queue[id] = time;
        // granted[target] = false;
        // granted[id] = true;
    }

    message_t message;
    message.time_stamp = time;
    message.type = type;
    message.origin = id;

    printf("[%d] SND %d -> %d %s\n", time, id, target,
    message_type_names[message.type]);
    write(fd[id][target][1], &message, sizeof(message_t));
}

void broadcast(message_type type) {
    for (int i = 0; i < NUM_PROC; ++i) {
        if (i != id)
            send_to(type, i);
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

  switch (message.type) {
  case REQUEST:
    request_queue[message.origin] = time;
    break;
  case GRANT:
    granted[message.origin] = true;
    break;
  case RELEASE:
    request_queue[message.origin] = MAXN;
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
  int retval = select(FD_SETSIZE, &rfds, NULL, NULL, &wt);

  if (retval == -1)
    perror("select()");
  else if (retval) {
    for (int i = 0; i < NUM_PROC; ++i) {
      if (i == id)
        continue;
      if (FD_ISSET(fd[i][id][0], &rfds))
        rcv_fd(fd[i][id][0]);
    }
  }
}

bool can_enter() {
    printf("GRANTED[%d]: ", id);
    for (int i = 0; i < NUM_PROC; ++i)
        printf(" %d", granted[i]);
    printf("\n");
    for (int i = 0; i < NUM_PROC; ++i) {
        if (request_queue[i] < request_queue[id])
            return false;
        if (!granted[i])
            return false;
    }
    return true;
}

void start_child() {
  for (int i = 0; i < NUM_PROC; ++i) {
    close(fd[id][i][0]);
    close(fd[i][id][1]);
    request_queue[i] = MAXN;
    granted[i] = false;
  }

  for (int i = 0; i < 1; ++i) {
    broadcast(REQUEST);
    for (;;) {
        rcv(1);
        if (can_enter()) {
            printf("[%d] Proc %d ulazi u kriticni\n", time, id);
            sleep(1);
            printf("[%d] Proc %d izlazi iz kriticni\n", time, id);
            broadcast(RELEASE);
        }
    }
  }
  exit(0);
}

int main(void) {

  for (size_t i = 0; i < NUM_PROC; i++) {
    for (size_t j = 0; j < NUM_PROC; j++) {
      if (pipe(fd[i][j]) == -1)
        printf("ERROR u otvaranju cjevovoda\n");
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
