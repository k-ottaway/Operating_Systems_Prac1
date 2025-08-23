/* Key Changes from the original minishell:
- added a linked list struct job to track background jobs.

- Extra helper functions such as:

  + new_job() — which adds job to list and prints [id] pid.

  + command() — which reconstructs command strings.

  + sigchld_handler() — which reaps finished background processes and prints the
status.

- installed a sigaction handler for SIGCHLD

- implemented stripping of trailing newline with strcspn that checks for empty
string or comment.

- EOF Handling by setting exit flag to high

- implemented a built-in cd (baseline executes everything via execvp).

- detects & as last token, removes it, and sets background = 1.

  + If background = 1, add to job list instead of waiting.

  + If background = 0, do a blocking waitpid.

- either waits (if in foreground) or add job (if in background). No "done"
message except via sigchld_handler.

- in final exit cleanup, loops with pause() until all background jobs are
reaped*/

#include <errno.h>  // used for error repeating
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */

// use a linked list to run the background jobs
struct job {
  int id;            // unique job no.
  pid_t pid;         // process id of the background job
  char cmd[NL];      // original command line string
  struct job *next;  // pointer for the next job in list
};

struct job *head = NULL;  // head of the linked list
int number_jobs = 0;      // job counter
int exit_flag = 0;        // flag for when stdin closes ( at end of shell )

/* Shell prompt */
void prompt(void) {
  // fprintf(stdout, "\n msh> ");
  fflush(stdout);
}

// Add background job to linked list and print ID and PID
void new_job(pid_t pid, char *cmdline) {
  struct job *job_ptr =
      malloc(sizeof(struct job));  // dynamically allocate job node
  if (!job_ptr) {                  // failure check
    perror("malloc failed");
    return;
  }

  job_ptr->id = number_jobs++;             // assign new job id
  job_ptr->pid = pid;                      // store the PID
  strncpy(job_ptr->cmd, cmdline, NL - 1);  // copy the command string
  job_ptr->cmd[NL - 1] = '\0';
  job_ptr->next = head;  // insert the job at the head of the list
  head = job_ptr;
  printf("[%d] %d\n", job_ptr->id, job_ptr->pid);  // print the job info
  fflush(stdout);
}

// command string from argv tokens (ignores trailing &)
void command(char *temp_store, size_t length, char *argv[]) {
  temp_store[0] = '\0';  // initialise the temproary storage to 0
  for (int i = 0; argv[i] != NULL; i++) {  // loop through the argument
    strncat(temp_store, argv[i],
            length - strlen(temp_store) - 1);  // add the next token to storage
    if (argv[i + 1] != NULL) {                 // if not the end of arg
      strncat(temp_store, " ",
              length - strlen(temp_store) - 1);  // add a new space
    }
  }
}

// child signal handler for background processes
void sigchld_handler(int sig) {
  int status;
  pid_t pid;

  // runs when one of the child processes terminates
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    struct job *prev = NULL;
    struct job *curr = head;

    // traverse the linked list to find job with pid
    while (curr) {
      if (curr->pid == pid) {
        if (WIFEXITED(status)) {  // if child exited normally print "Done"
          printf("[%d]+ Done %s\n", curr->id, curr->cmd);
        } else if (WIFSIGNALED(
                       status)) {  // if child killed by signal, print "Killed"
          printf("[%d]+ Killed %s\n", curr->id, curr->cmd);
        }
        fflush(stdout);  // ensure output prints immediately

        if (prev)                   // remove finished job from linked list
          prev->next = curr->next;  // link the previous job to the next job
        else
          head = curr->next;  // if the job is the first, update head

        struct job *tmp = curr;  // store the current job to free memory
        curr = curr->next;       // move to next job before freeing
        free(tmp);
        continue;  // continue iteration
      }
      prev = curr;
      curr = curr->next;
    }
  }
}

int main(int argc, char *argv[], char *envp[]) {
  char *argv_tok[NV];         // array to store tokens
  char *delimiter = " \t\n";  // seperates the tokens
  int i;

  // set up the sigchild handler to reap background jobs
  struct sigaction sig_handle;
  sig_handle.sa_handler = sigchld_handler;  // handler fucntio
  sigemptyset(&sig_handle.sa_mask);  // no signals to be blocked during handler
  sig_handle.sa_flags = SA_RESTART;  // restart the interrupted syscalls
  if (sigaction(SIGCHLD, &sig_handle, NULL) == -1) {
    perror("sigaction failed");  // error print
    exit(1);
  }

  // main shell loop
  while (1) {
    prompt();
    if (fgets(line, NL, stdin) == NULL) {  // read a line from stdin
      if (feof(stdin)) {                   // if EOF set exit flag high
        exit_flag = 1;
        break;
      }
      perror("fgets failed");  // if error, print
      continue;
    }

    line[strcspn(line, "\n")] = 0;  // Removes trailing newline from input

    if (line[0] == '#' || line[0] == '\0')
      continue;  // Ignore any empty lines or comments

    argv_tok[0] =
        strtok(line, delimiter);  // Tokenizes the input line into array

    for (i = 1; i < NV; i++) {  // continue tokenising until there are no more
                                // tokens or limit is reached
      argv_tok[i] = strtok(NULL, delimiter);
      if (argv_tok[i] == NULL) break;
    }

    if (argv_tok[0] == NULL) continue;  // If no command entered, continue

    // Handle the built-in cd command
    if (strcmp(argv_tok[0], "cd") == 0) {
      const char *dir = argv_tok[1];
      // if no argument, use HOME or root
      if (!dir) {
        dir = getenv("HOME");
        if (!dir) dir = "/";
      }
      if (chdir(dir) != 0) {  // change the directory and print error message
        perror("chdir failed");
      }
      continue;
    }

    // Check if last token is '&' for background job
    int background = 0;
    if (i > 0 && argv_tok[i - 1] && strcmp(argv_tok[i - 1], "&") == 0) {
      background = 1;
      argv_tok[i - 1] = NULL;  // remove & from tokens
    }

    // fork child process to execute command
    pid_t pid = fork();
    if (pid < 0) {  // If forking fails, print an error message and continue
      perror("fork failed");
      continue;
    }

    if (pid == 0) {
      // execute command using execp
      if (execvp(argv_tok[0], argv_tok) ==
          -1) {  // If execvp fails, print error message and exit child
        perror("execvp failed");
        exit(1);
      }
    } else {
      if (background) {  // parent process
        char fullcmd[NL];
        command(fullcmd, NL,
                argv_tok);      // build a command string for job tracking
        new_job(pid, fullcmd);  // add new job to the linked list
      } else {
        // wait for the child process to finish
        if (waitpid(pid, NULL, 0) == -1) {
          perror("waitpid failed");
        }
      }
    }
  }

  // after EOF, wait for all background jobs to finish before exiting
  while (head != NULL) {
    pause();
  }
  return 0;
}