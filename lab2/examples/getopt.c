#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct astruct {
  int x; 
  int y;
};


int main(int argc, char **argv) {
  char opt;
  int value = 1234;
  struct astruct *sp = NULL;
  

  while ((opt = getopt(argc, argv, "a:bc")) != -1) {
    switch (opt) {
      case 'a':
        printf("apples: %s\n", optarg);
        value = atoi(optarg);
        break;

      case 'b':
        printf("bananas\n");
        break;

      case 'c':
        printf("couscous\n");
        break;
    }
  }
  printf("value is %d\n", value);
  sp->x = 4;
  sp->y = 3;
}
