#include <assert.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum { true = 1, false = 0 } bool;
#define MAX_PROCESS_NUM 100
#define MAX_LINE 1024 // 每行最大字节数
const char *path = "/proc";

struct procnode {
  char name[15];
  int ppid;
  int pid;
  int childnum;
  struct procnode **childs;
};

static int isdigitstr(char *str) {
  return (strspn(str, "0123456789") == strlen(str));
}

bool compsubstr(const char *src, const char *target) {
  int len = strlen(target);
  for (int i = 0; i < len; i++) {
    if (src[i] != target[i]) {
      return false;
    }
  }
  return true;
}

char *getValue(const char *strLine) {
  int n = strlen(strLine);
  int i = 0;
  for (; i < n; i++) {
    if (strLine[i] == ':')
      break;
  }
  // 中间的空白是'\t'，末尾是'\n'
  char *value = malloc(15);
  int vstart = i + 2;
  strncpy(value, strLine + vstart, n - vstart - 1);
  value[n - vstart] = '\0';

  return value;
}

void initrecur(struct procnode *root, struct procnode *p) {
  if (root == NULL)
    return;
  if (p->ppid == root->pid) { // 是子节点，递归结束
    root->childnum++;
    root->childs = realloc(root->childs, root->childnum * sizeof(p));
    root->childs[root->childnum - 1] = p;
    return;
  } else {
    // 遍历所有子节点
    for (int i = 0; i < root->childnum; i++) {
      initrecur(root->childs[i], p);
    }
  }
}

void drawrecur(struct procnode *p, uint32_t depth, bool bshowPID) {
  if (p == NULL)
    return;
  for (int i = 0; i < depth; i++) {
    printf("    ");
  }
  printf("%s", p->name);
  if (bshowPID)
    printf("(%d)", p->pid);
  printf("\n");

  // 遍历所有子节点
  for (int i = 0; i < p->childnum; i++) {
    drawrecur(p->childs[i], depth + 1, bshowPID);
  }
}

int main(int argc, char *argv[]) {
  char kind = '0';
  if (argc == 2) {
    if ((kind = getopt(argc, argv, "Vnp")) == -1)
	    exit(0);
  }

  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);

  DIR *dir;
  struct dirent *ptr;

  if ((dir = opendir(path)) == NULL) {
    perror("Open dir error");
    exit(1);
  }

  struct procnode plist[MAX_PROCESS_NUM];
  int index = 0;

  // 获取plist
  while ((ptr = readdir(dir)) != NULL) {
    // 对读到的单个文件进行处理
    if (isdigitstr(ptr->d_name) && ptr->d_type == DT_DIR) {
      char subpath[strlen(path) + 1];
      strcpy(subpath, path);
      strcat(subpath, "/");
      strcat(subpath, ptr->d_name);
      strcat(subpath, "/status");
      FILE *fp = fopen(subpath, "r");
      if (fp) {
        // 用fscanf, fgets等函数读取
        char strLine[MAX_LINE];
        // 初始化
        plist[index].childnum = 0;
        plist[index].childs = NULL;

        for (int j = 0; j < 6; j++) { // 读6行
          fgets(strLine, MAX_LINE, fp);

          if (compsubstr(strLine, "Name")) {
            char *value = getValue(strLine);
            strcpy(plist[index].name, value);
            free(value);
          } else if (compsubstr(strLine, "Pid")) {
            char *value = getValue(strLine);
            plist[index].pid = atoi(value);
            free(value);
          } else if (compsubstr(strLine, "PPid")) {
            char *value = getValue(strLine);
            plist[index].ppid = atoi(value);
            free(value);
          }
        }
        fclose(fp);
        // printf("%s,%d,%d\n", plist[index].name, plist[index].pid,
        //        plist[index].ppid);
      } else {
        // 错误处理
        perror("Open status file error");
        exit(1);
      }
      index++;
    }
  }
  int numproc = index; // 进程数量

  // 处理数据结构，pid已经是文件系统排好序的
  struct procnode root = {"init", 0, 1};
  for (int i = 1; i < numproc; i++) {
    initrecur(&root, &plist[i]);
  }

  switch (kind) {
  case '0':
    drawrecur(&root, 0, false);
    break;
  case 'p':
    drawrecur(&root, 0, true);
    break;
  case 'n':
    drawrecur(&root, 0, false);
    break;
  case 'V':
    printf("Version\n");
    break;
  default:
    printf("error: format is wrong!\n");
    return 0;
  }

  return 0;
}
