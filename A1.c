#include <stdio.h>
#include <unistd.h>

#define X 20

int main()
{
  int i = 0, p = getpid();
  printf("PID <%d> diz: Ola\n", p);

  do
  {
    printf("PID <%d> diz: Ola novamente (%d)\n", p, ++i);
    sleep(2);
  } while (i < X);
}
