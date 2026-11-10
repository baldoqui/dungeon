#pragma once

#include <string>
#include <vector>

#include "TileMap.hpp"
#include "Vector2D.hpp"

/**
 * O labirinto do jogo, lido de um arquivo de texto (assets/labirinto.txt):
 *
 *    #  parede
 *    .  corredor com pontinho
 *    o  corredor com poção
 *       (espaço) corredor vazio
 *    H  onde o herói começa
 *    P  curral dos monstros (o herói não entra)
 *    -  porta do curral (só os monstros passam)
 *
 * Uma linha com as duas pontas abertas vira túnel: sair por um lado
 * é entrar pelo outro.
 *
 * Posições em TILES (col, row), como no resto do jogo.
 */

// O que pode estar no chão de um tile.
enum Item { NADA = 0, PONTINHO = 1, POCAO = 2 };

class Labirinto {
public:
   bool carrega (const std::string &arquivo);

   int largura () const { return m_largura; }
   int altura  () const { return m_altura; }

   // Túnel: a coluna -1 é a última, e a coluna "largura" é a 0.
   int dentroDoMapa (int col) const;

   bool livreParaHeroi   (int col, int row) const;
   bool livreParaMonstro (int col, int row) const;
   bool noCurral         (int col, int row) const;

   int  item     (int col, int row) const;
   void tiraItem (int col, int row);
   int  itensRestantes () const { return m_restantes; }
   void recolocaItens ();

   // Uma int por tile (Item), para a Tela desenhar.
   const std::vector<int> &itens () const { return m_itens; }

   Vector2D inicioHeroi;
   Vector2D saidaDoCurral;          // tile logo acima da porta
   std::vector<Vector2D> curral;    // tiles de dentro do curral

   // Paredes (WALL_TILE) e corredores (EMPTY_TILE), para desenhar e para o A*.
   // Curral e porta são corredores aqui: os monstros precisam passar.
   TileMap mapa;

private:
   std::vector<std::string> m_linhas;
   std::vector<int> m_itensDoComeco;
   std::vector<int> m_itens;
   int m_restantes {0};
   int m_largura {0};
   int m_altura  {0};

   char letra (int col, int row) const;
};
