#!/usr/bin/env python3

import sys
import time


def main():
    inicio = time.perf_counter()

    total_linhas = 0
    unicas = {}

    for linha in sys.stdin:
        linha = linha.strip()

        total_linhas += 1

        unicas[linha] = unicas.get(linha, 0) + 1

    fim = time.perf_counter()

    tempo = fim - inicio

    print(f"Total de linhas    : {total_linhas}")
    print(f"Linhas únicas      : {len(unicas)}")
    print(f"Tempo              : {tempo:.2f} s")

    if tempo > 0:
        print(f"Taxa               : " f"{total_linhas/tempo:,.0f} linhas/s")


if __name__ == "__main__":
    main()
