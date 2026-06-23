#!/usr/bin/env python3

import sys
import io
import time
import zstandard as zstd


def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} arquivo.zst")
        sys.exit(1)

    arquivo = sys.argv[1]

    inicio = time.perf_counter()

    total_linhas = 0
    unicas = {}

    with open(arquivo, "rb") as fh:
        dctx = zstd.ZstdDecompressor(max_window_size=2147483648)

        with dctx.stream_reader(fh) as reader:
            text_stream = io.TextIOWrapper(reader, encoding="utf-8")

            for linha in text_stream:
                linha = linha.strip()

                total_linhas += 1

                unicas[linha] = unicas.get(linha, 0) + 1

    fim = time.perf_counter()

    tempo = fim - inicio

    print(f"Arquivo            : {arquivo}")
    print(f"Total de linhas    : {total_linhas}")
    print(f"Linhas únicas      : {len(unicas)}")
    print(f"Tempo              : {tempo:.2f} s")

    if tempo > 0:
        print(f"Taxa               : " f"{total_linhas/tempo:,.0f} linhas/s")


if __name__ == "__main__":
    main()
