#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include "defn.h"
#define LINELEN 200000

int ctime();
int shif = 0;
extern int isdigit(int number_to_check);
extern int argc;

int builtin(char *line, int num, int outfd){

  int ix = 0;
  int ix1;
  int tex = 0;
  int shiftby;
  char temp[LINELEN];
  char *value;

  // exit
  if(strcmp("exit", line) == 0){
    if(num > 1){
      int exitno = line[ix+5] - '0';
      exit(exitno);
      }
      else{
        exit(0);
      }
    }

  // shift
  else if(strcmp("shift", line) == 0){
    ix1 = 6;
    if(isdigit(line[ix1])){
      shiftby = atoi(&line[ix1]);
      if(shif+shiftby < argc && shiftby > 0){
        shif += shiftby;
      }
      else{
        fprintf(stderr, "shift\n");
        return(2);
      }
    }
    else{
      shiftby = 1;
      if(shif+shiftby < argc && shiftby > 0){
        shif += shiftby;
      }
      else{
        fprintf(stderr, "shift\n");
        return(2);
      }
    }
    return(0);
  }

  // unshift
  else if(strcmp("unshift", line) == 0){
    ix1 = 8;
    if(isdigit(line[ix1])){
      shiftby = atoi(&line[ix1]);
      if(shif-shiftby < argc && shiftby > 0){
        shif -= shiftby;
      }
      else{
        fprintf(stderr, "shift\n");
        return(2);
      }
    }
    else{
      shif = 0;
    }
    return(0);
  }

  //sstat
  else if(strcmp(line, "sstat") == 0){
    ix1 = 6;
    struct passwd *pw;
    struct stat sb;
    struct group  *gr;

    while(line[ix1] != '\0'){
      tex = 0;
      while(line[ix1] != ' ' && line[ix1] != '\0'){
        temp[tex] = line[ix1];
        tex++;
        ix1++;
      }
      ix1++;
      temp[tex] = '\0';

      // write to pipe
      if(outfd != 1){
        if(stat(temp, &sb) == -1){
          perror("stat");
        }
        else{
          dprintf(outfd, "%s ", temp);
          pw = getpwuid(sb.st_uid);
          gr = getgrgid(sb.st_gid);
          if((pw->pw_name) != NULL){
            dprintf(outfd, "%s ", pw->pw_name);
          }
          else{
            dprintf(outfd, "%ld ", (long) sb.st_uid);
          }
          if((gr->gr_name )!= NULL){
            dprintf(outfd, "%s ", gr->gr_name);
          }
          else{
            dprintf(outfd, "%ld ", (long) sb.st_gid);
          }
          dprintf(outfd, (S_ISDIR(sb.st_mode)) ? "d" : "-");
          dprintf(outfd, (sb.st_mode & S_IRUSR) ? "r" : "-");
          dprintf(outfd, (sb.st_mode & S_IWUSR) ? "w" : "-");
          dprintf(outfd, (sb.st_mode & S_IXUSR) ? "x" : "-");
          dprintf(outfd, (sb.st_mode & S_IRGRP) ? "r" : "-");
          dprintf(outfd, (sb.st_mode & S_IWGRP) ? "w" : "-");
          dprintf(outfd, (sb.st_mode & S_IXGRP) ? "x" : "-");
          dprintf(outfd, (sb.st_mode & S_IROTH) ? "r" : "-");
          dprintf(outfd, (sb.st_mode & S_IWOTH) ? "w" : "-");
          dprintf(outfd, (sb.st_mode & S_IXOTH) ? "x " : "- ");
          dprintf(outfd, "%d ", sb.st_mode);
          dprintf(outfd, "%ld ", (long) sb.st_nlink);
          dprintf(outfd, "%lld ", (long long) sb.st_size);
          dprintf(outfd, "%d ", ctime(&sb.st_mtime));
          dprintf(outfd, "\n");
        }
      }
      // just stdout
      else{
        if(stat(temp, &sb) == -1){
          perror("stat");
        }
        else{
          printf("%s ", temp);
          pw = getpwuid(sb.st_uid);
          gr = getgrgid(sb.st_gid);
          if((pw->pw_name) != NULL){
            printf("%s ", pw->pw_name);
          }
          else{
            printf("%ld ", (long) sb.st_uid);
          }
          if((gr->gr_name )!= NULL){
            printf("%s ", gr->gr_name);
          }
          else{
            printf("%ld ", (long) sb.st_gid);
          }
          printf((S_ISDIR(sb.st_mode)) ? "d" : "-");
          printf((sb.st_mode & S_IRUSR) ? "r" : "-");
          printf((sb.st_mode & S_IWUSR) ? "w" : "-");
          printf((sb.st_mode & S_IXUSR) ? "x" : "-");
          printf((sb.st_mode & S_IRGRP) ? "r" : "-");
          printf((sb.st_mode & S_IWGRP) ? "w" : "-");
          printf((sb.st_mode & S_IXGRP) ? "x" : "-");
          printf((sb.st_mode & S_IROTH) ? "r" : "-");
          printf((sb.st_mode & S_IWOTH) ? "w" : "-");
          printf((sb.st_mode & S_IXOTH) ? "x " : "- ");
          printf("%d ", sb.st_mode);
          printf("%ld ", (long) sb.st_nlink);
          printf("%lld ", (long long) sb.st_size);
          printf("%d ", ctime(&sb.st_mtime));
          printf("\n");
        }
      }
    }
    return(0);
  }

  // cd
  else if(strcmp("cd", line) == 0){
    if(num == 1){
      ix1 = 3;
      while(line[ix1] != ' '){
        temp[tex] = line[ix1];
        ix1++;
        tex++;
      }
      temp[tex] = '\0';
      if(chdir(temp) != 0){
        perror("chdir");
        return(2);
      }
      return(0);
    }
    else{
      chdir(getenv("HOME"));
      return(0);
    }
  }

  // envunset
  else if(strcmp("envunset", line) == 0){
    ix1 = 9;
    while(line[ix1] != ' '){
      temp[tex] = line[ix1];
      ix1++;
      tex++;
    }
    temp[tex] = '\0';
    if(temp != NULL){
      unsetenv(temp);
      return(0);
    }
    else{
      fprintf(stderr, "unsetenv\n");
      return(2);
    }
  }

  // envset
  else if(strcmp("envset", line) == 0){
    ix1 = 7;
    while(line[ix1] != ' '){
      temp[tex] = line[ix1];
      ix1++;
      tex++;
    }
    temp[tex] = '\0';
    if(temp != NULL && line[ix1+1] != ' '){
      value = &line[ix1+1];
      setenv(temp, value, 1);
      return(0);
    }
    else{
      fprintf(stderr, "No name or val for setenv\n");
      return(2);
    }
  }

  // not a built in
  return(-1);
}
