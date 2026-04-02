#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define X 4

static void tratador_alarme(int s)
{
  printf("\nsinal recebido <%d>\n", s);
}

int main()
{
  // Se estas linhas forem comentadas, o programa não saberá tratar o sinal de alarme,
  // pois não estava esperando por ele, fazendo o programa parar em um "Alarm clock"
  struct sigaction sa;
  sa.sa_handler = tratador_alarme;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);

  printf("Ola\n");
  printf("vai setar alarme\n");
  alarm(X);

  int i = 0;
  char sair;

  do
  {
    printf("Ola novamente (%d)\n", ++i);
    printf("Digite s para sair: ");
    // scanf(" %c", &sair);
    // O sinal atrapalha o scanf fazendo pular a leitura, é preciso garantir que um char seja lido
    while(scanf(" %c", &sair) != 1);
    printf("Voce digitou <%c>\n", sair);
  } while (sair != 's');
}
