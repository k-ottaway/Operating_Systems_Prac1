/*********************************************************************
   miniShell with basic job control
   - supports background jobs (&)
   - tracks job IDs
   - reports when jobs finish with "[n]+ Done command"
********************************************************************/

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NV 20      /* max tokens */
#define NL 100     /* input buffer size */
#define MAXJOBS 50 /* max background jobs tracked */

struct job {
  int id;
  pid_t pid;
  char cmdline[NL];
  int active;
};

static struct job jobs[MAXJOBS];
static int next_job_id = 1;
static char line[NL];

/* prompt */
static void prompt(void) {
  // fprintf(stdout, "\n msh> ");
  fflush(stdout);
}

/* find job by pid */
static struct job *find_job(pid_t pid) {
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].active && jobs[i].pid == pid) return &jobs[i];
  }
  return NULL;
}

/* add job */
static void add_job(pid_t pid, const char *cmdline) {
  for (int i = 0; i < MAXJOBS; i++) {
    if (!jobs[i].active) {
      jobs[i].active = 1;
      jobs[i].pid = pid;
      jobs[i].id = next_job_id++;
      strncpy(jobs[i].cmdline, cmdline, NL - 1);
      jobs[i].cmdline[NL - 1] = '\0';
      printf("[%d] %d\n", jobs[i].id, pid);
      fflush(stdout);
      return;
    }
  }
}

/* remove job and report */
static void remove_job(pid_t pid) {
  struct job *j = find_job(pid);
  if (j) {
    printf("[%d]+ Done                 %s\n", j->id, j->cmdline);
    fflush(stdout);
    j->active = 0;
  }
}

/* SIGCHLD handler */
static void sigchld_handler(int sig) {
  (void)sig;
  int saved_errno = errno;
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    remove_job(pid);
  }
  errno = saved_errno;
}

/* built-in cd */
static void builtin_cd(char *const v[]) {
  const char *target = NULL;
  if (v[1] == NULL || strcmp(v[1], "~") == 0) {
    target = getenv("HOME");
    if (!target) {
      fprintf(stderr, "cd: HOME not set\n");
      return;
    }
  } else {
    target = v[1];
  }
  if (chdir(target) == -1) {
    perror("chdir");
  }
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
  }

  while (1) {
    prompt();
    if (fgets(line, NL, stdin) == NULL) {
      if (feof(stdin)) break;
      perror("fgets");
      continue;
    }
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;

    /* tokenize */
    char *v[NV];
    char *sep = " \t\n";
    v[0] = strtok(line, sep);
    if (!v[0]) continue;
    int i;
    for (i = 1; i < NV; i++) {
      v[i] = strtok(NULL, sep);
      if (!v[i]) break;
    }
    int background = 0;
    if (i > 0 && v[i - 1] && strcmp(v[i - 1], "&") == 0) {
      background = 1;
      v[i - 1] = NULL;
    }

    /* built-ins */
    if (strcmp(v[0], "cd") == 0) {
      builtin_cd(v);
      continue;
    }
    if (strcmp(v[0], "exit") == 0) break;

    /* fork */
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      continue;
    }
    if (pid == 0) {
      execvp(v[0], v);
      perror("execvp");
      _exit(127);
    }

    if (!background) {
      if (waitpid(pid, NULL, 0) == -1) perror("waitpid");
      printf("%s done \n", v[0]);
    } else {
      /* reconstruct command string for job table */
      char cmdline_buf[NL] = {0};
      for (int j = 0; v[j]; j++) {
        strcat(cmdline_buf, v[j]);
        if (v[j + 1]) strcat(cmdline_buf, " ");
      }
      add_job(pid, cmdline_buf);
    }
  }
  return 0;
}
