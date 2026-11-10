#pragma once

#include <memory>
#include <string>

#include <SFML/Window/Keyboard.hpp>

#include "Ator.hpp"
#include "Labirinto.hpp"
#include "Monstros.hpp"
#include "PathFind.hpp"

/**
 * As regras do jogo. Não abre janela nem desenha nada: o Main.cpp liga
 * isto à janela (Tela). Assim dá para testar as regras sem tela
 * (test/JogoTest.cpp).
 *
 * Pontos: pontinho 10, poção 50, monstro vulnerável 200, 400, 800, 1600
 * (dobra a cada monstro comido com a mesma poção).
 */

enum Etapa {
   PRONTO,          // pausa antes de começar (ou recomeçar) a vida
   JOGANDO,
   PEGO,            // um monstro pegou o herói
   FASE_COMPLETA,   // comeu tudo
   FIM_DE_JOGO,     // acabaram as vidas; R recomeça
};

const int NUMERO_DE_MONSTROS = 4;

struct Jogo {
   Labirinto lab;
   std::unique_ptr<PathFind> caminho;

   Ator    heroi;
   Direcao desejo;               // última seta apertada
   Monstro monstros[NUMERO_DE_MONSTROS];

   Etapa etapa {PRONTO};
   float pausa {0};              // segundos que faltam na etapa atual

   int pontos  {0};
   int recorde {0};
   int vidas   {0};
   int fase    {1};

   bool  dispersao {true};       // monstros indo para os cantos?
   float tempoDoModo {0};        // até trocar dispersão <-> perseguição
   float tempoVulneravel {0};    // > 0 enquanto a poção vale
   int   comidosNaPocao {0};     // para dobrar os pontos

   // Carrega o labirinto e cria o A*. false se o arquivo não abrir.
   bool prepara (const std::string &arquivoDoLabirinto);

   void comecaJogo ();           // pontos zerados, 3 vidas, fase 1
   void atualiza (float dt);     // um quadro
   void tecla (sf::Keyboard::Key k);

   std::string textoDoPlacar () const;
   float duracaoDaPocao () const;

private:
   void comecaVida ();           // herói e monstros nos lugares iniciais
   void jogaUmQuadro (float dt);
   void moveHeroi (float dt);
   void moveMonstro (Monstro &m, float dt);
   void comeItem ();
   void verificaEncontros ();
   void atualizaCores ();
   float velocidadeDosMonstros () const;
};
