# Trabalho II – Gerenciamento de Memória

### Integrantes

- Alex Sander Condines dos Santos (169622)
- Pedro Garcia Machado (169591)

### Algoritmo escolhido

- FIFO (First In, First Out)

### Entrada

- Diretório o arquivo;
- Tamanho da memória e página especificados em TAMANHO [B|KB|GB|TB]. Ex.: 1024 MB.

## Objetivo

Implementar um programa que calcule o total de faltas de página utilizando dois dos algoritmos vistos em aula:

- Algoritmo Ótimo;
- Um segundo algoritmo à escolha do grupo.

O programa receberá como entrada um arquivo texto onde cada linha representa um acesso à memória. No arquivo é indicado a qual endereço se refere cada acesso. Uma mesma página pode ser acessada mais de uma vez, e portanto pode aparecer repetidas vezes (em endereços diferentes ou não) no arquivo.

Para esta simulação, vamos assumir que só existam páginas de instruções e páginas de dados e que o tamanho de cada página é um parâmetro do programa.

---

## Entradas do Programa

O programa deve receber:

1. Arquivo contendo os acessos à memória;
2. Tamanho da memória física simulada;
3. Tamanho da página.

Na avaliação, o programa será testado com diferentes tamanhos de memória física, como:

- 1 GB
- 128 MB
- 16 MB
- 8 KB

### Exemplo de arquivo de entrada

```text
0x60a1c35d8209
0x60a1c35d82d8
0x7ee8df4ed010
0x60a1c35d8209
0x60a1c35d82d8
0x7ee8df4ed011
0x60a1c35d8209
0x60a1c35d82d8
0x7ee8df4ed012
...
```

Cada linha representa um endereço de memória acessado.

---

## Saída Esperada

Como resposta, o programa deve exibir:

- Quantas páginas cabem na memória física;
- Número total de endereços acessados;
- Número total de faltas de página;
- Eficiência do segundo algoritmo;
- Quantidade de páginas distintas presentes no arquivo de entrada.

### Observação

Cada página acessada precisa ser trazida pelo menos uma vez para a memória física. Portanto, para qualquer algoritmo, existe um limite mínimo de pelo menos uma falta de página para cada página acessada.

As faltas adicionais provocadas por uma mesma página são decorrentes de eventual limitação da memória física, ou seja, uma página precisou ser removida da memória física para ceder lugar a outra.

---

## Algoritmos

Cada grupo deve implementar:

### Obrigatório

- Ótimo

### Escolher um dos seguintes

1. NRU
2. FIFO
3. Segunda Chance
4. Relógio
5. LRU
6. NFU
7. Envelhecimento (Aging)
8. Conjunto de Trabalho
9. WSClock

### Observação

Dependendo do segundo algoritmo escolhido, outros parâmetros de entrada podem ser necessários, por exemplo:

- Frequência de interrupções do Sistema Operacional;
  - Pode ser simulada em termos de número de instruções executadas.

- Largura do contador (número de bits).

---

## Requisitos Recomendados

Além dos requisitos mínimos, podem ser implementadas funcionalidades adicionais:

- Opção para listar o número de vezes que cada página foi carregada;
- Estimativa do tamanho necessário em bytes para armazenar a tabela de páginas (de 1 nível);
- Modo didático:
  - Para uma memória física pequena (por exemplo, 32 KB), mostrar o estado da memória física ao longo da execução da simulação;

- Nos algoritmos NRU, FIFO, Segunda Chance, Relógio, LRU, NFU e Aging:
  - Implementar parâmetro para escolher política de substituição local ou global.

### Critério de Avaliação

- Trabalhos que implementem apenas os requisitos mínimos terão nota oscilando próximo à média (7);
- Para alcançar notas maiores será necessário implementar parte dos requisitos recomendados;
- Para trabalhos desenvolvidos em grupo espera-se mais funcionalidades do que em trabalhos feitos individualmente.

---

## Dados de Teste Disponibilizados

O professor disponibilizou arquivos contendo rastros reais de acessos à memória para utilização nos testes da simulação.

### Importante

Os arquivos disponibilizados no AVA estão compactados em formato GZ.

Esses arquivos são relativamente pequenos e, caso a memória simulada comporte mais de aproximadamente 10 páginas, a simulação tende a apresentar resultados muito semelhantes independentemente do algoritmo utilizado.

Os arquivos brutos em TXT contendo os acessos de memória possuem mais de **300 GB**.

Por esse motivo, foram disponibilizadas versões compactadas utilizando **Zstandard (ZSTD)**, um algoritmo moderno de compressão de dados.

### Formas recomendadas de leitura

#### Opção 1 – Ler pela entrada padrão

Utilizar o `zstdcat` para descompactar o arquivo e enviar os dados diretamente para o programa:

```bash
zstdcat acessos-Demo0.txt.zst | python3 read_stdin.py
```

#### Opção 2 – Ler diretamente o arquivo compactado

Utilizar uma biblioteca capaz de ler arquivos ZSTD sem extração prévia, por exemplo o pacote `zstandard` para Python:

```bash
python3 read.py acessos-Demo0.txt.zst
```

### Atenção

**Não extrair os arquivos diretamente para SSD ou HD**, a menos que exista pelo menos 1 TB de espaço livre disponível.

O recomendado é utilizar uma das abordagens acima, que permitem processar os dados sem realizar a extração completa em disco.

### Cenários de Teste Sugeridos

Testar a simulação com memórias de diferentes tamanhos:

- Alguns KB;
- 4 MB;
- 1 GB.

Com essas entradas e esses tamanhos de memória deve ser possível observar diferenças entre o algoritmo Ótimo e o algoritmo escolhido pelo grupo.

### Material Complementar

O professor disponibilizou:

- Novos conjuntos de dados;
- Programas de exemplo em Python para servir como base para o desenvolvimento do trabalho.

Link disponibilizado:

https://filesender.rnp.br/?s=download&token=ed98aff7-0ba6-4347-b0eb-3bac6078c067

---

## Regras do Trabalho

### Grupos

O trabalho pode ser realizado em grupos de até 3 alunos.

### Entrega

Entregar:

- Código-fonte comentado;
- Instruções de compilação e execução.

### Prazo

Entrega pelo AVA até:

**23/06/2026 às 23h55**

### Peso

- 50% da N2 (2º bimestre)

### Sistema Operacional

O Sistema Operacional alvo para o qual a aplicação será escrita é de livre escolha.

Segundo o professor, isso não fará muita diferença neste trabalho.

### Apresentação

Os alunos devem ser capazes de demonstrar o funcionamento do programa em sala de aula.

Todos os integrantes do grupo devem estar aptos a apresentar qualquer parte do trabalho.

### Atenção

Todas as entregas serão submetidas à verificação de plágio por meio de medida de similaridade de código.
