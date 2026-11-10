#pragma once

#include <string>

#include "Actor.hpp"
#include "Maze.hpp"
#include "PathFind.hpp"

/**
 * The four monsters. All of them walk the shortest path (A*, PathFind) to
 * a target tile; what changes between them is how they pick the target:
 *
 *   Brute    (demon)  goes straight for the hero.
 *   Ambusher (bat)    aims 4 tiles ahead of the hero, to cut him off.
 *   Flanker  (spider) takes the point 2 tiles ahead of the hero and
 *            mirrors the Brute around it: the two close in from both sides.
 *   Coward   (slime)  chases from afar, but runs to its corner once it
 *            gets close (less than 8 tiles).
 *
 * In scatter mode each one heads to its corner. When frightened (after
 * the hero drinks a potion), they run away from the hero.
 */

enum Personality { BRUTE, AMBUSHER, FLANKER, COWARD };

enum MonsterState {
   IN_PEN,        // waiting for its turn to leave
   LEAVING_PEN,   // going from the pen to the door
   ROAMING,       // out in the maze
};

struct Monster {
   Actor        actor;
   Personality  personality;
   std::string  name;
   std::string  image;            // file in assets/kenney/
   Vector2D     corner;           // target in scatter mode
   MonsterState state {IN_PEN};
   float        wait {0};         // seconds until it leaves the pen
   bool         frightened {false};
};

// What the monsters need to know about the rest of the game.
struct Situation {
   Vector2D  hero;
   Direction heroDir;
   Vector2D  brute;     // the Flanker uses the Brute's position
   bool      scatter;   // true = everyone to their corners
};

// Tile the monster wants to reach now.
Vector2D targetOf (const Monster &m, const Situation &s);

// Open tile (outside the pen) closest to any target, which may have
// fallen off the map or onto a wall.
Vector2D validTarget (const Vector2D &target, const Maze &maze);

// First step of the shortest path (A*) to the target.
Direction directionTowards (const Actor &actor, const Vector2D &target,
                            const PathFind &pathFinder, const IsOpen &isOpen);

// Frightened: turn to whichever side gets farthest from the hero.
Direction fleeDirection (const Actor &actor, const Vector2D &hero,
                         const IsOpen &isOpen);
