#include "Monstros.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "Global.hpp"


static float distancia2 (const Vector2D &a, const Vector2D &b) {
   return (a - b).length_squared();
}

Vector2D alvoDe (const Monstro &m, const Situacao &s) {

   if (s.dispersao)
      return m.canto;

   Vector2D frente (s.dirHeroi.dx, s.dirHeroi.dy);

   switch (m.personalidade) {
      case BRUTO:
         return s.heroi;

      case ESPREITA:
         return s.heroi + frente * 4;

      case CERCO: {
         Vector2D pivo = s.heroi + frente * 2;
         return pivo * 2 - s.bruto;
      }

      case MEDROSO:
         if (distancia2(m.ator.npc.body.position, s.heroi) > 8 * 8)
            return s.heroi;
         return m.canto;
   }
   return s.heroi;
}


Vector2D alvoValido (const Vector2D &alvo, const Labirinto &lab) {

   int col = std::clamp((int) std::round(alvo.x), 0, lab.largura() - 1);
   int row = std::clamp((int) std::round(alvo.y), 0, lab.altura()  - 1);

   auto serve = [&](int c, int r) {
      return lab.livreParaHeroi(c, r) && c >= 0 && c < lab.largura();
   };

   if (serve(col, row))
      return Vector2D(col, row);

   // Procura em anéis cada vez maiores em volta do alvo.
   int maior = std::max(lab.largura(), lab.altura());
   for (int raio = 1; raio < maior; ++raio) {
      Vector2D melhor;
      float melhorDist = std::numeric_limits<float>::max();
      for (int r = row - raio; r <= row + raio; ++r)
         for (int c = col - raio; c <= col + raio; ++c) {
            bool naBorda = std::abs(r - row) == raio || std::abs(c - col) == raio;
            if (!naBorda || !serve(c, r))
               continue;
            float d = distancia2(Vector2D(c, r), alvo);
            if (d < melhorDist) {
               melhorDist = d;
               melhor = Vector2D(c, r);
            }
         }
      if (melhorDist < std::numeric_limits<float>::max())
         return melhor;
   }
   return Vector2D(col, row);
}


// PathFind trabalha em pixels e devolve o centro de cada tile do caminho.
static Vector2D centroEmPixels (int col, int row) {
   return Vector2D((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
}

// Quando não há caminho: qualquer direção livre, evitando dar meia-volta.
static Direcao qualquerLivre (const Ator &ator, const Livre &livre) {
   for (Direcao d: DIRECOES)
      if (d != ator.dir.contraria() && livre(ator.col() + d.dx, ator.row() + d.dy))
         return d;
   return ator.dir.contraria();
}

Direcao direcaoPara (const Ator &ator, const Vector2D &alvo,
                     const PathFind &caminho, const Livre &livre) {

   std::vector<Vector2D> rota =
      caminho.findPath(centroEmPixels(ator.col(), ator.row()),
                       centroEmPixels((int) alvo.x, (int) alvo.y));

   if (rota.size() < 2)
      return qualquerLivre(ator, livre);

   // rota[0] é o tile onde o monstro já está; rota[1] é o próximo.
   int proxCol = (int) std::floor(rota[1].x / TILE_SIZE);
   int proxRow = (int) std::floor(rota[1].y / TILE_SIZE);
   Direcao d { (proxCol > ator.col()) - (proxCol < ator.col()),
               (proxRow > ator.row()) - (proxRow < ator.row()) };

   // O A* pode andar na diagonal; aqui só se anda na horizontal ou na
   // vertical, então fica com a parte que estiver livre.
   if (d.dx != 0 && d.dy != 0) {
      if (livre(ator.col() + d.dx, ator.row()))
         d.dy = 0;
      else
         d.dx = 0;
   }

   if (!livre(ator.col() + d.dx, ator.row() + d.dy))
      return qualquerLivre(ator, livre);

   return d;
}


Direcao direcaoDeFuga (const Ator &ator, const Vector2D &heroi,
                       const Livre &livre) {

   Direcao melhor = PARADO;
   float   maisLonge = -1;

   for (Direcao d: DIRECOES) {
      if (d == ator.dir.contraria() || !livre(ator.col() + d.dx, ator.row() + d.dy))
         continue;
      float dist = distancia2(Vector2D(ator.col() + d.dx, ator.row() + d.dy), heroi);
      if (dist > maisLonge) {
         maisLonge = dist;
         melhor = d;
      }
   }

   // Beco sem saída: a única saída é voltar.
   if (melhor.parado())
      return ator.dir.contraria();
   return melhor;
}
