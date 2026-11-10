#include "Ator.hpp"

#include <algorithm>
#include <cmath>

// Quanto x e y podem fugir de um inteiro e ainda contar como "no centro".
static const float QUASE = 0.001f;


int Ator::col () const { return (int) std::floor(npc.body.position.x + 0.5f); }
int Ator::row () const { return (int) std::floor(npc.body.position.y + 0.5f); }

bool Ator::noCentro () const {
   const Vector2D &p = npc.body.position;
   return std::abs(p.x - std::round(p.x)) < QUASE &&
          std::abs(p.y - std::round(p.y)) < QUASE;
}

void Ator::colocaEm (const Vector2D &tile) {
   npc.body.position = tile;
   npc.body.velocity = Vector2D(0, 0);
   dir = PARADO;
}


void anda (Ator &ator, float dt, const Labirinto &lab,
           const Livre &livre, const Escolha &escolhe) {

   Vector2D &p = ator.npc.body.position;
   float resta = ator.velocidade * dt;

   // Um quadro pode atravessar mais de um centro de tile (quadro lento);
   // a cada centro, o ator pode virar.
   for (int centros = 0; centros < 8 && resta > 0; ++centros) {

      if (ator.noCentro()) {
         p = Vector2D(std::round(p.x), std::round(p.y));

         // Túnel: passou da borda, aparece do outro lado.
         if (p.x < 0)                p.x = (float) (lab.largura() - 1);
         if (p.x > lab.largura() - 1) p.x = 0;

         ator.dir = escolhe(ator);
         if (ator.dir.parado() ||
             !livre(ator.col() + ator.dir.dx, ator.row() + ator.dir.dy)) {
            ator.dir = PARADO;
            return;
         }
      }

      if (ator.dir.parado())
         return;

      // Quanto falta até o próximo centro, na direção do movimento.
      float coordenada = ator.dir.dx != 0 ? p.x : p.y;
      int   sentido    = ator.dir.dx != 0 ? ator.dir.dx : ator.dir.dy;
      float proximo    = sentido > 0 ? std::floor(coordenada + QUASE) + 1
                                     : std::ceil (coordenada - QUASE) - 1;
      float falta      = std::abs(proximo - coordenada);

      float passo = std::min(resta, falta);
      p += Vector2D((float) ator.dir.dx, (float) ator.dir.dy) * passo;
      resta -= passo;
   }
}
