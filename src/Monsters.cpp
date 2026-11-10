#include "Monsters.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "Global.hpp"


static float distance2 (const Vector2D &a, const Vector2D &b) {
   return (a - b).length_squared();
}

Vector2D targetOf (const Monster &m, const Situation &s) {

   if (s.scatter)
      return m.corner;

   Vector2D ahead (s.heroDir.dx, s.heroDir.dy);

   switch (m.personality) {
      case BRUTE:
         return s.hero;

      case AMBUSHER:
         return s.hero + ahead * 4;

      case FLANKER: {
         Vector2D pivot = s.hero + ahead * 2;
         return pivot * 2 - s.brute;
      }

      case COWARD:
         if (distance2(m.actor.npc.body.position, s.hero) > 8 * 8)
            return s.hero;
         return m.corner;
   }
   return s.hero;
}


Vector2D validTarget (const Vector2D &target, const Maze &maze) {

   int col = std::clamp((int) std::round(target.x), 0, maze.width()  - 1);
   int row = std::clamp((int) std::round(target.y), 0, maze.height() - 1);

   auto usable = [&](int c, int r) {
      return c >= 0 && c < maze.width() && maze.isOpenForHero(c, r);
   };

   if (usable(col, row))
      return Vector2D(col, row);

   // Search in growing rings around the target.
   int largest = std::max(maze.width(), maze.height());
   for (int radius = 1; radius < largest; ++radius) {
      Vector2D best;
      float bestDist = std::numeric_limits<float>::max();
      for (int r = row - radius; r <= row + radius; ++r)
         for (int c = col - radius; c <= col + radius; ++c) {
            bool onRing = std::abs(r - row) == radius || std::abs(c - col) == radius;
            if (!onRing || !usable(c, r))
               continue;
            float d = distance2(Vector2D(c, r), target);
            if (d < bestDist) {
               bestDist = d;
               best = Vector2D(c, r);
            }
         }
      if (bestDist < std::numeric_limits<float>::max())
         return best;
   }
   return Vector2D(col, row);
}


// PathFind works in pixels and returns the centre of each tile on the path.
static Vector2D centreInPixels (int col, int row) {
   return Vector2D((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
}

// When there is no path: any open direction, avoiding turning back.
static Direction anyOpen (const Actor &actor, const IsOpen &isOpen) {
   for (Direction d: DIRECTIONS)
      if (d != actor.dir.opposite() && isOpen(actor.col() + d.dx, actor.row() + d.dy))
         return d;
   return actor.dir.opposite();
}

Direction directionTowards (const Actor &actor, const Vector2D &target,
                            const PathFind &pathFinder, const IsOpen &isOpen) {

   std::vector<Vector2D> path =
      pathFinder.findPath(centreInPixels(actor.col(), actor.row()),
                          centreInPixels((int) target.x, (int) target.y));

   if (path.size() < 2)
      return anyOpen(actor, isOpen);

   // path[0] is the monster's own tile; path[1] is the next one.
   int nextCol = (int) std::floor(path[1].x / TILE_SIZE);
   int nextRow = (int) std::floor(path[1].y / TILE_SIZE);
   Direction d { (nextCol > actor.col()) - (nextCol < actor.col()),
                 (nextRow > actor.row()) - (nextRow < actor.row()) };

   // A* may step diagonally; actors only move horizontally or vertically,
   // so keep whichever part is open.
   if (d.dx != 0 && d.dy != 0) {
      if (isOpen(actor.col() + d.dx, actor.row()))
         d.dy = 0;
      else
         d.dx = 0;
   }

   if (!isOpen(actor.col() + d.dx, actor.row() + d.dy))
      return anyOpen(actor, isOpen);

   return d;
}


Direction fleeDirection (const Actor &actor, const Vector2D &hero,
                         const IsOpen &isOpen) {

   Direction best = STOP;
   float farthest = -1;

   for (Direction d: DIRECTIONS) {
      if (d == actor.dir.opposite() || !isOpen(actor.col() + d.dx, actor.row() + d.dy))
         continue;
      float dist = distance2(Vector2D(actor.col() + d.dx, actor.row() + d.dy), hero);
      if (dist > farthest) {
         farthest = dist;
         best = d;
      }
   }

   // Dead end: the only way out is back.
   if (best.isStop())
      return actor.dir.opposite();
   return best;
}
