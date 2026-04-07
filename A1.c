#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define X 20

static void tratador_sinal(int s)
{
  printf("\nsinal recebido <%d>\n", s);
}

int main()
{
  struct sigaction sa;
  sa.sa_handler = tratador_sinal;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTERM, &sa, NULL); // Trata o sinal SIGTERM <15>
  sigaction(SIGINT, &sa, NULL); // Trata o sinal SIGINT <2>

  int i = 0, p = getpid();
  printf("PID <%d> diz: Ola\n", p);

  do
  {
    printf("PID <%d> diz: Ola novamente (%d)\n", p, ++i);
    sleep(2);
  } while (i < X);
}
