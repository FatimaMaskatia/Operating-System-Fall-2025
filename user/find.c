#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

char buf[512];
int exec_flag = 0;
char *cmdargv[16];
int cmdargc = 0;

//
// regex functions copied from grep.c
//
int matchhere(char *re, char *text);
int matchstar(int c, char *re, char *text);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do {  // must look even if string is empty
    if(matchhere(re, text))
      return 1;
  } while(*text++ != '\0');
  return 0;
}

int
matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

int
matchstar(int c, char *re, char *text)
{
  do{ // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}

//
// helper: get last element of path
//
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
  return buf;
}

//
// recursive find
//
void
find(char *path, char *pattern)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(match(pattern, fmtname(path))){   // use regex here
      if(exec_flag){
        int pid = fork();
        if(pid == 0){
          char *argv[MAXARG];
          int i;
          for(i=0; i<cmdargc; i++){
            argv[i] = cmdargv[i];
          }
          argv[i++] = path;
          argv[i] = 0;
          exec(argv[0], argv);
          fprintf(2, "find: exec %s failed\n", argv[0]);
          exit(1);
        } else {
          wait(0);
        }
      } else {
        printf("%s\n", path);
      }
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, pattern);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "Usage: find <path> <pattern> [-exec cmd ...]\n");
    exit(1);
  }

  char *path = argv[1];
  char *pattern = argv[2];

  if(argc > 3 && strcmp(argv[3], "-exec") == 0){
    exec_flag = 1;
    cmdargc = argc-4;
    for(int i=0; i<cmdargc; i++)
      cmdargv[i] = argv[4+i];
  }

  find(path, pattern);
  exit(0);
}
