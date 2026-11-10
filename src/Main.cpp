// Dungeon: jogo de labirinto com perseguição.
//
// Este arquivo só liga as regras (Jogo) à janela (Tela). As regras ficam em
// Jogo.cpp, o movimento em Ator.cpp, os monstros em Monstros.cpp e o
// labirinto em Labirinto.cpp.

#include <algorithm>
#include <iostream>

#include "Jogo.hpp"
#include "Tela.hpp"

// Arte: Tiny Dungeon, de Kenney (CC0); ver README.
#define ARTE ASSETS_DIR "kenney/"


int main () {

   Jogo jogo;
   if (!jogo.prepara (ASSETS_DIR "labirinto.txt")) {
      std::cerr << "Não consegui carregar labirinto.txt\n";
      return 1;
   }

   Tela tela (jogo.lab.mapa);

   // Imagens. Os personagens são desenhados na ordem em que são
   // adicionados: o herói por último, por cima dos monstros.
   for (auto &m: jogo.monstros) {
      m.ator.npc.sprite (ARTE + m.imagem);
      tela.adiciona (&m.ator.npc);
   }
   jogo.heroi.npc.sprite (ARTE "cavaleiro.png");
   tela.adiciona (&jogo.heroi.npc);

   // Monstro vulnerável vira fantasma.
   sf::Texture fantasma;
   if (!fantasma.loadFromFile (ARTE "fantasma.png")) {
      std::cerr << "Não consegui carregar fantasma.png\n";
      return 1;
   }

   tela.imagemDaParede (ARTE "parede.png");
   tela.imagemDoChao   (ARTE "chao.png");
   tela.itens          (&jogo.lab.itens());
   tela.corDoItem      (PONTINHO, sf::Color(255, 210, 90), 0.1f);   // moedinha
   tela.imagemDoItem   (POCAO, ARTE "pocao.png");

   tela.aoApertarTecla ([&](sf::Keyboard::Key k) { jogo.tecla(k); });

   tela.aoPassarQuadro ([&](float dt) {
      // Um quadro muito longo (janela arrastada, primeiro quadro...) faria
      // todo mundo pular vários tiles de uma vez.
      jogo.atualiza (std::min(dt, 0.05f));

      for (auto &m: jogo.monstros)
         m.ator.npc.m_sprite.setTexture (m.vulneravel ? fantasma : m.ator.npc.m_texture);

      tela.textoDoPlacar (jogo.textoDoPlacar());

      // Barra = tempo que falta de poção.
      tela.barraDoPlacar (jogo.tempoVulneravel > 0
                          ? jogo.tempoVulneravel / jogo.duracaoDaPocao() : -1);
   });

   jogo.comecaJogo();

   // Espaço em cima do labirinto para o placar.
   tela.margemSuperior (3.5f);
   if (!tela.abre (900, 1000, "Dungeon"))
      return 1;

   if (!tela.fonteDoPlacar (ASSETS_DIR "DejaVuSans-Bold.ttf"))
      std::cerr << "Não consegui carregar a fonte; o jogo segue sem placar.\n";

   tela.roda();
   return 0;
}
