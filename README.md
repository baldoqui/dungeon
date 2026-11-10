# Dungeon

Jogo de labirinto com perseguição em C++17 e SFML 2: um cavaleiro junta as moedas da masmorra fugindo de quatro monstros. As poções deixam os monstros vulneráveis.

![Tela do jogo](docs/tela.png)

Setas movem, R recomeça depois do fim de jogo.

## Compilar

Precisa de SFML 2, CMake e do repo [disciplina-de-jogos](https://github.com/baldoqui/disciplina-de-jogos) na mesma pasta.

```sh
cmake -S . -B build && cmake --build build
./build/dungeon
```

## Referências

- Arte: [Tiny Dungeon](https://kenney.nl/assets/tiny-dungeon), de Kenney, licença CC0 ([`assets/kenney/License.txt`](assets/kenney/License.txt)).
- Fonte: [DejaVu Sans](https://dejavu-fonts.github.io/) ([licença](assets/DejaVuSans-LICENSE.txt)).
- Biblioteca: [SFML](https://www.sfml-dev.org/).

Código sob a [licença MIT](LICENSE).
