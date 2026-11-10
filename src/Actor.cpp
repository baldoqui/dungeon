#include "Actor.hpp"

#include <algorithm>
#include <cmath>

// How far x and y may be from an integer and still count as "at the centre".
static const float NEARLY = 0.001f;


int Actor::col () const { return (int) std::floor(npc.body.position.x + 0.5f); }
int Actor::row () const { return (int) std::floor(npc.body.position.y + 0.5f); }

bool Actor::atTileCenter () const {
   const Vector2D &p = npc.body.position;
   return std::abs(p.x - std::round(p.x)) < NEARLY &&
          std::abs(p.y - std::round(p.y)) < NEARLY;
}

void Actor::placeAt (const Vector2D &tile) {
   npc.body.position = tile;
   npc.body.velocity = Vector2D(0, 0);
   dir = STOP;
}


void move (Actor &actor, float dt, const Maze &maze,
           const IsOpen &isOpen, const ChooseDirection &choose) {

   Vector2D &p = actor.npc.body.position;
   float left = actor.speed * dt;

   // A frame can cross more than one tile centre (slow frame);
   // the actor may turn at each of them.
   for (int centres = 0; centres < 8 && left > 0; ++centres) {

      if (actor.atTileCenter()) {
         p = Vector2D(std::round(p.x), std::round(p.y));

         // Tunnel: past the edge, show up on the other side.
         if (p.x < 0)                 p.x = (float) (maze.width() - 1);
         if (p.x > maze.width() - 1)  p.x = 0;

         actor.dir = choose(actor);
         if (actor.dir.isStop() ||
             !isOpen(actor.col() + actor.dir.dx, actor.row() + actor.dir.dy)) {
            actor.dir = STOP;
            return;
         }
      }

      if (actor.dir.isStop())
         return;

      // Distance to the next centre, along the movement.
      float coord = actor.dir.dx != 0 ? p.x : p.y;
      int   sign  = actor.dir.dx != 0 ? actor.dir.dx : actor.dir.dy;
      float next  = sign > 0 ? std::floor(coord + NEARLY) + 1
                             : std::ceil (coord - NEARLY) - 1;
      float gap   = std::abs(next - coord);

      float step = std::min(left, gap);
      p += Vector2D((float) actor.dir.dx, (float) actor.dir.dy) * step;
      left -= step;
   }
}
