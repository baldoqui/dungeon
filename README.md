# Dungeon

A maze-chase game in C++17 and SFML 2: a knight collects the coins of a dungeon while four monsters chase him. Potions make the monsters vulnerable.

![Screenshot](docs/screenshot.png)

Arrow keys move; R starts a new game after game over.

## Build

Needs SFML 2, CMake and the [disciplina-de-jogos](https://github.com/baldoqui/disciplina-de-jogos) repo in the same parent folder.

```sh
cmake -S . -B build && cmake --build build
./build/dungeon
```

## Credits

- Art: [Tiny Dungeon](https://kenney.nl/assets/tiny-dungeon) by Kenney, CC0 ([`assets/kenney/License.txt`](assets/kenney/License.txt)).
- Font: [DejaVu Sans](https://dejavu-fonts.github.io/) ([license](assets/DejaVuSans-LICENSE.txt)).
- Library: [SFML](https://www.sfml-dev.org/).

Code under the [MIT license](LICENSE).
