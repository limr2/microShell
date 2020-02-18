#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "defn.h"
#include <dirent.h>
#include <ctype.h>
#define LINELEN 200000
#define WAIT 1
#define NOWAIT 0
#define EXPAND 2
#define NOEXPAND 4

int processline (char *line, int infd, int outfd, int flags);
extern int argc;
extern char **argv;
extern int shif;
extern int check;

int expand(char *orig, char *new, int newsize){

  int ix = 0;
  int ix1 = 0; // for new array
  int ipid;
  int tex = 0; // pointer to start of command
  int state = 0;
  char temp[LINELEN]; // store command
  char* envname;
  int rv;
  DIR* dtry;
  struct dirent *dp;
  int filecopied = 0;
  char filename[255];
  int len;
  int status;
  char *cmd;
  int numrequest;

  while(orig[ix] != '\0'){
    if(orig[ix] == ' '){
      new[ix1] = orig[ix];
      ix++;
      ix1++;
    }
    else if(state == 0){
      // has *
      if(orig[ix] == '*'){
          // *
          if(ix == 0 || orig[ix-1] == ' '){
            ix++;
            if(orig[ix] == ' '){
              if((dtry = opendir(".")) == NULL){
                exit(0);
              }
              while((dp = readdir(dtry)) != NULL){
                strncpy(filename, dp->d_name, 254);
                filename[254] = '\0';
                if(strcmp(filename, "/") != 0){
                  if(filename[0] != '.'){
                    rv = snprintf(&new[ix1], newsize, "%s", dp->d_name);
                    ix1 += rv;
                    if(newsize != 0){
                      new[ix1] = ' ';
                      ix1++;
                      newsize--;
                    }
                    else{
                      return(-1);
                    }
                  }
                }
                else{
                  fprintf(stderr, "path has /\n");
                  return(0);
                }
              }
              closedir(dtry);
              ix++;
            }
            // *words
            else{
              while(orig[ix] != ' '){
                temp[tex] = orig[ix];
                tex++;
                ix++;
              }
              temp[tex] = '\0';
              if((dtry = opendir(".")) == NULL){
                perror("\ndir unopen\n");
                exit(0);
              }
              while((dp = readdir(dtry)) != NULL){
                strncpy(filename, dp->d_name, 254);
                filename[254] = '\0';
                if(strcmp(filename, temp) == 0){
                  rv = snprintf(&new[ix1], newsize, "%s", dp->d_name);
                  ix1 += rv;
                  filecopied++;
                }
              }
              if(filecopied == 0){
                if(newsize != 0){
                  new[ix1] = '*';
                  ix1++;
                  newsize--;
                  rv = snprintf(&new[ix1], newsize, "%s", temp);
                  ix1+= rv;
                }
                else{
                  return(-1);
                }
              }
              closedir(dtry);
            }
          }
          // just /* or other cases
          else{
            new[ix1-1] = '*';
            ix++;
          }
        }
      // $
      else if(orig[ix] == '$'){
        // $n
        if(isdigit(orig[ix+1])){
          numrequest = atoi(&orig[ix+1]);
          ix++;
          if(numrequest == 0){
            if(newsize != 0){
              rv = snprintf(&new[ix1], newsize, "%s", argv[1]);
              ix1+= rv;
              while(isdigit(orig[ix])){
                ix++;
              }
            }
            else{
              return(-1);
            }
          }
          else{
            if((numrequest-1+shif) > -1 && (numrequest-1+shif) < (argc+shif-1)){
              if(newsize != 0){
                rv = snprintf(&new[ix1], newsize, "%s", argv[numrequest+1+shif]);
                ix1+= rv;
                while(isdigit(orig[ix])){
                  ix++;
                }
              }
              else{
                return(-1);
              }
            }
            else{
              if(newsize != 0){
                new[ix1] = ' ';
                ix1++;
                newsize--;
                while(isdigit(orig[ix])){
                  ix++;
                }
              }
              else{
                return(-1);
              }
            }
          }
        }
        // $#
        else if(orig[ix+1] == '#'){
          rv = snprintf(&new[ix1], newsize, "%d", argc-shif-1);
          ix += 2;
          ix1 += rv;
        }

        ////////////////////////// new code ////////////////////////////////////
        // $?
        else if(orig[ix+1] == '?'){
          rv = snprintf(&new[ix1], newsize, "%d", check);
          ix += 2;
          ix1 += rv;
        }
        // $()
        else if(orig[ix+1] == '('){
          ix += 2;
          cmd = &orig[ix];

          int parand_count = 1;
          while(parand_count != 0 && orig[ix] != '\0'){
            if(orig[ix] == ')'){
              parand_count--;
              ix++;
            }
            else if(orig[ix] == '('){
              parand_count++;
              ix++;
            }
            else{
              ix++;
            }
          }
          // NOT MATCHING ()
          if(parand_count != 0){
            fprintf(stderr, "no )\n");
            return(-1);
          }
          else{
            orig[ix-1] = '\0';
          }

          // PIPE //
          int pipefd[2];
          if(pipe(pipefd) == -1){
            fprintf(stderr, "pipe\n");
            return -1;
          }

          // PROCESSLINE //
          rv = processline(cmd, 0, pipefd[1], NOWAIT | EXPAND);
          if(rv < 0){
            return -1;
          }
          close(pipefd[1]);

          // READ //
          while(read(pipefd[0], &new[ix1], 1) > 0 && newsize != 0){
            newsize--;
            ix1++;
          }
          if(newsize == 0){
            fprintf(stderr, "buffer overflow\n");
            return -1;
          }
          ix1++;

          // REMOVE \n //
          len = strlen(new);
          if(new[len-1] == '\n'){
            new[len-1] = '\0';
          }
          tex = 0;
          while(new[tex] != '\0'){
            if(new[tex] == '\n'){
              new[tex] = ' ';
            }
            tex++;
          }

          // clean //
          close(pipefd[0]);
          orig[ix-1] = ')';

          // $? //
          if (wait (&status) < 0) {
            break;
          }
          else{
            if(WIFEXITED(status)){
              check = WEXITSTATUS(status);
            }
            else if(WIFSIGNALED(status) && WTERMSIG(status) != SIGINT){
              check = WTERMSIG(status) + 128;
            }
          }
        }
        ////////////////////////// new code ////////////////////////////////////

        // ${COMMAND}
        else if(orig[ix+1] == '{'){
          state = 1;
          ix += 2;
        }
        // double $$
        else if(orig[ix+1] == '$'){
          ipid = getpid();
          rv = snprintf(&new[ix1], newsize, "%d", ipid);
          ix += 2;
          ix1 += rv;
        }
        // just $
        else if(newsize != 0){
          new[ix1] = orig[ix];
          ix1++;
          ix++;
          newsize--;
        }
        else{
          return(-1);
        }
      }
      // char
      else if(newsize != 0){
        new[ix1] = orig[ix];
        ix1++;
        ix++;
        newsize--;
      }
      else{
        new[ix1-1] = '\0';
        return(-1);
      }
    }
    // ${COMMAND} processing
    else{
      if(orig[ix] == '}'){
        temp[tex] = '\0';

        envname = getenv(temp);
        tex = 0;
        if(envname != NULL){
          if(newsize != 0){
            new[ix1] = *envname;
            ix1++;
            newsize--;
          }
          else{
            new[ix1-1] = '\0';
            return(-1);
          }
        }
        ix++;
        state = 0;
      }
      // env command
      else{
        if(tex != LINELEN){
          temp[tex] = orig[ix];
          ix++;
          tex++;
        }
        else{
          new[ix1-1] = '\0';
          return(-1);
        }
      }
    }
  }
  new[ix1] = '\0';

  // {} NOT MATCHED
  if(state == 1){
    fprintf(stderr,"{} error\n");
    new[ix1-1] = '\0';
    return(-1);
  }
  // EXPANDED W NO ERROR
  return(0);
}
