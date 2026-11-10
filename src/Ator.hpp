#pragma once

#include <functional>

#include "Labirinto.hpp"
#include "NPC.hpp"

/**
 * Movimento em grade, contínuo: o ator desliza de um tile para o próximo,
 * e só pode trocar de direção quando está exatamente no centro de um tile
 * (a não ser para dar meia-volta, que pode a qualquer momento).
 *
 * Como na Tela, a posição (npc.body.position) está em TILES e
 * marca o canto superior esquerdo; "centro de um tile" quer dizer posição
 * com x e y inteiros.
 */

struct Direcao {
   int dx {0};
   int dy {0};

   bool parado () const { return dx == 0 && dy == 0; }
   Direcao contraria () const { return Direcao{-dx, -dy}; }
   bool operator== (const Direcao &o) const { return dx == o.dx && dy == o.dy; }
   bool operator!= (const Direcao &o) const { return !(*this == o); }
};

const Direcao PARADO   { 0,  0};
const Direcao ESQUERDA {-1,  0};
const Direcao DIREITA  { 1,  0};
const Direcao CIMA     { 0, -1};
const Direcao BAIXO    { 0,  1};
const Direcao DIRECOES[4] = {CIMA, ESQUERDA, BAIXO, DIREITA};

struct Ator {
   NPC     npc;
   Direcao dir;
   float   velocidade {0};   // tiles por segundo

   // Tile mais perto do ator (onde ele "está" para comer, colidir...).
   int col () const;
   int row () const;

   bool noCentro () const;
   void colocaEm (const Vector2D &tile);
};

// Pode entrar no tile (col, row)?
using Livre = std::function<bool (int col, int row)>;

// No centro de um tile, para onde ir agora? (PARADO = fica parado)
using Escolha = std::function<Direcao (const Ator &ator)>;

/**
 * Anda velocidade * dt tiles. A cada centro de tile pelo caminho, pergunta
 * a direção para escolhe(); se ela estiver bloqueada, o ator para ali.
 * Cuida também do túnel (sai de um lado, aparece do outro).
 */
void anda (Ator &ator, float dt, const Labirinto &lab,
           const Livre &livre, const Escolha &escolhe);
