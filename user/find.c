#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

char* fmtname(char *path) {
  static char buf[DIRSIZ+1];
  char *p;

  for(p = path+strlen(path); p >= path && *p != '/'; p--);
  p++;
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}

void find(char *path, char *filename, int exec_flag, char *cmd) {
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
    if(strcmp(fmtname(path), filename) == 0){
      if(exec_flag){
        int pid = fork();
        if(pid == 0){
          char *argv[4];
          argv[0] = cmd;
          argv[1] = path;
          argv[2] = 0;
          exec(argv[0], argv);
          exit(0);
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
      if(de.inum == 0) continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, filename, exec_flag, cmd);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if(argc < 3){
    fprintf(2, "usage: find <path> <filename> [-exec cmd]\n");
    exit(1);
  }

  int exec_flag = 0;
  char *cmd = 0;
  if(argc > 4 && strcmp(argv[3], "-exec") == 0){
    exec_flag = 1;
    cmd = argv[4];
  }

  find(argv[1], argv[2], exec_flag, cmd);
  exit(0);
}
