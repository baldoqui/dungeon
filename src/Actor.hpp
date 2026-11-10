#pragma once

#include <functional>

#include "Maze.hpp"
#include "NPC.hpp"

/**
 * Continuous grid movement: an actor slides from one tile to the next and
 * can only change direction exactly at the centre of a tile (except for
 * turning back, which is allowed at any moment).
 *
 * As in the Screen, the position (npc.body.position) is in TILES and marks
 * the top-left corner; "centre of a tile" means integer x and y.
 */

struct Direction {
   int dx {0};
   int dy {0};

   bool isStop () const { return dx == 0 && dy == 0; }
   Direction opposite () const { return Direction{-dx, -dy}; }
   bool operator== (const Direction &o) const { return dx == o.dx && dy == o.dy; }
   bool operator!= (const Direction &o) const { return !(*this == o); }
};

const Direction STOP  { 0,  0};
const Direction LEFT  {-1,  0};
const Direction RIGHT { 1,  0};
const Direction UP    { 0, -1};
const Direction DOWN  { 0,  1};
const Direction DIRECTIONS[4] = {UP, LEFT, DOWN, RIGHT};

struct Actor {
   NPC       npc;
   Direction dir;
   float     speed {0};   // tiles per second

   // Nearest tile (where the actor "is" for eating, colliding...).
   int col () const;
   int row () const;

   bool atTileCenter () const;
   void placeAt (const Vector2D &tile);
};

// Can the actor enter tile (col, row)?
using IsOpen = std::function<bool (int col, int row)>;

// At the centre of a tile, where to go now? (STOP = stay)
using ChooseDirection = std::function<Direction (const Actor &actor)>;

/**
 * Moves speed * dt tiles. At every tile centre on the way it asks
 * choose() for a direction; if that one is blocked, the actor stops there.
 * Also handles the tunnel (leave on one side, show up on the other).
 */
void move (Actor &actor, float dt, const Maze &maze,
           const IsOpen &isOpen, const ChooseDirection &choose);
