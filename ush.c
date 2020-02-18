/* CS 352 -- Miro Shell!
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001
 *   Modified January 6, 2003
 *   Modified January 8, 2017
 *
 * Rose Lim
 * Started 01/10/2019
 * CSCI347 Winter
 * Assignment 1
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "defn.h"
#include <signal.h>

#define LINELEN 200000
#define WAIT 1
#define NOWAIT 0
#define EXPAND 2
#define NOEXPAND 4

int processline (char *line, int infd, int outfd, int flags);
char ** arg_parse (char *line, int *argcptr);

int argc;
char **argv;
int check = -1;
static int sigintcheck = -1;
static int forkcheck = -1;
static FILE *fptr;

/* Shell main */
int
main (int argnum, char **argvector)
{
    char   buffer [LINELEN];
    int    len;
    argc = argnum;
    argv = argvector;

    // READ FILE //
    if(argnum > 1){
      fptr = fopen(argvector[1], "r");
      if(fptr == NULL){
        fprintf(stderr, "file not opened\n");
        // ADD SYSTEM ERROR /////////////
        exit(127);
      }
    }

    // STDINPUT //
    else{
      fptr = stdin;
    }

    while (1) {
      if(argnum == 1){
        fprintf (stderr, "%% ");
      }
      if (fgets (buffer, LINELEN, fptr) != buffer)
        break;

      // COMMENTS //
	    len = strlen(buffer);
      int ix = 0;
      while(ix < len){
        if(buffer[ix] == '$'){
        ix+=2;
        }
        else if(buffer[ix] == '#'){
          buffer[ix] = 0;
          ix++;
          break;
        }
        else{
          ix++;
        }
      }

      // NEW LINE //
      if(buffer[len-1] == '\n'){
        buffer[len-1] = 0;
      }

      /* Run it ... */
      processline (buffer, 0, 1, WAIT | EXPAND);
      }

      // READ ERROR //
      if (!feof(fptr)){
        perror ("read");
      }

      return 0;
}

int processline (char *line, int infd, int outfd, int flags)
{
    pid_t  cpid;
    int    status;
    int    ix = 0;
    char   **parsearray;
    char   new[LINELEN];
    int    expcheck;
    int    buildcheck;
    int    killcheck;
    int    nix = 0;
    char   temp[LINELEN];
    int    tex = 0;
    int    pipe1[2];
    int    rv;
    int    savepipe = 0;

    // ZOMBIE //
    if(forkcheck == 0){
      while(waitpid(getpid(), &status, WNOHANG) > 0){
       printf("killed zombie\n");
      }
    }

    // EXPAND //
    if(flags & EXPAND){
      expcheck = expand(line, new, LINELEN);
      if(expcheck == -1){
        return -1;
      }
    }
    // COPY LINE TO NEWLINE //
    else{
      strcpy(new,line);
    }

    // PIPES //
    if(strchr(new, '|') != NULL){
      nix = 0;
      // go through all arguments
      while(new[nix] != '\0'){
        tex = 0;
        // store one arg
        while(new[nix] != '|' && new[nix] != '\0'){
          temp[tex] = new[nix];
          tex++;
          nix++;
        }
        nix++;
        temp[tex] = '\0';

        // not last arg
        if(new[nix-1] == '|'){
          if(pipe(pipe1) == -1){
            fprintf(stderr, "pipe\n");
            return -1;
          }
          rv = processline(temp, savepipe, pipe1[1], NOWAIT | NOEXPAND);
          if(savepipe != 0){
            close(savepipe);
          }
          savepipe = pipe1[0];
          close(pipe1[1]);
        }
        // last arg (wait if requested flag?)
        else{
          if(flags & WAIT){
            rv = processline(temp, savepipe, outfd, WAIT | NOEXPAND);
          }
          else{
            rv = processline(temp, savepipe, outfd, NOWAIT | NOEXPAND);
          }
          close(savepipe);
          return 0;
        }
        if(rv < 0){
          fprintf(stderr, "didnt write to pipe");
          return -1;
        }
      }
    }

    // ARG PARSE //
    parsearray = arg_parse(new, &ix);
    if(parsearray == NULL){
      return -1;
    }

    // BUILT IN? //
    buildcheck = builtin(new, ix, outfd);

    // NEW PROCESS because not built in //
    if(buildcheck == -1){
      // FORK //
      cpid = fork();
      forkcheck = 0;
      if (cpid < 0) {
        // FORK FAIL //
        perror ("fork\n");
        free(parsearray);
        return -1;
      }

      // CHILD? //
      if (cpid == 0) {

        // REDIRECT STDIN //
        if(infd != 0){
          dup2(infd, 0);
        }

        // REDIRECT STDOUT //
        if(outfd != 1){
          dup2(outfd, 1);
        }

        execvp (parsearray[0], parsearray);

        // EXEC RETURNED, not successful //
        perror ("exec");
        fclose(fptr);
        exit (127);
      }

      // WAIT? //
      if(flags & WAIT){
        // PARENT, wait for child //
        if (waitpid(-1, &status, 0) < 0) {
          // WAIT FAIL //
          perror ("wait");
        }

        // WAIT SUCCESS //
        else{

          // REPORT EXIT //
          // checks if prog is exited under control
          if(WIFEXITED(status)){
            // returns exit status
            check = WEXITSTATUS(status);
          }
          // checks if program is exited bc signal
          else if(WIFSIGNALED(status) && WTERMSIG(status) != SIGINT){
            // returns terminating signal
            fprintf(stderr, "%s", strsignal(WTERMSIG(status)));
            if(WCOREDUMP(status)){
              fprintf(stderr, " (core dumped)");
            }
            fprintf(stderr, "\n");
            check = WTERMSIG(status) + 128;
          }

          // SIGNAL PROCESSING //
          else if(WIFSIGNALED(status) && WTERMSIG(status) == SIGINT){
            // ignore signal
            signal(SIGINT, SIG_IGN);
            // if not eintr (interrupt by function call)
            if(errno != EINTR){
              // send the signal
              killcheck = kill(getpid(), SIGINT);
              if(killcheck != -1){
                // at success, store in global variable
                sigintcheck = 0;
              }
            }
          }

          // NO CHILD RUNNING //
          free(parsearray);
          return 0;
        }
      }
      // NO WAIT
      else{
        return cpid;
      }
    }

    // BUILT IN //
    else{
      if(buildcheck == 0){
        check = 0;
      }
      else{
        check = 1;
      }
    }
    free(parsearray);
    return 0;
  }

char ** arg_parse(char *line, int *argcptr) {
  int ax;
  char **ixarray;
  int in;
  int out;
  in = 0;
  while(line[in] != '\0'){
    while(line[in] == ' '){
      in++;
    }
    if(line[in] != '\0'){
      (*argcptr)++;
      while(line[in] != '\0' && line[in] != ' '){
        if(line[in] == '"'){
          in++;
          while(line[in] != '"' && line[in] != '\0') {
            in++;
          }
          if(line[in] != '"'){
            return(0);
          }
          else{
            in++;
          }
        }
        else{
          in++;
        }
      }
    }
  }
  (*argcptr)++;

  ixarray = (char**) malloc(sizeof(char*)*(*argcptr));
  // MALLOC ERROR
  if(ixarray == NULL){
    *argcptr = -1;
    return(NULL);
  }
  // MALLOC SUCCESS
  else {
    in = 0;
    out = 0;
    ax = 0;
    while(line[in] != '\0'){
      while(line[in] == ' '){
        in++;
      }
      // not space and not end of char is ARG
      if(line[in] != '\0'){
        ixarray[ax] = &line[out];
        ax++;
        // go through arg
        while(line[in] != ' ' && line[in] != '\0'){
          if(line[in] == '"'){
            in++;
            // while not " or end of string
            while(line[in] != '"' && line[in] != '\0') {
              line[out] = line[in];
              out++;
              in++;
            }
            // current character is not "
            if(line[in] != '"'){
              in++;
            }
            else{
              in++;
            }
          }
          // if not quote, copy directly
          else{
            line[out] = line[in];
            out++;
            in++;
          }
        }
        if(in == out && line[in] == ' '){
          in++;
          line[out] = '\0';
          out++;
        }
        else{
          line[out] = '\0';
          out++;
        }
      }
    }
  ixarray[ax] = NULL;
  return(ixarray);
  }
}
