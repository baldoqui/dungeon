#pragma once

#include <functional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "NPC.hpp"
#include "TileMap.hpp"

/**
 * The game window: draws the whole maze, the items, the characters, the
 * HUD and the menus, and calls the game every frame and on every key press.
 *
 * Units: character positions (npc.body.position) are in TILES, top-left
 * corner; each tile is TILE_SIZE x TILE_SIZE in the world, and any image
 * is stretched to fit one tile.
 *
 * Layout: the HUD is a strip at the top of the window; the maze fills the
 * rest, as large as it fits without distortion. Menus cover everything.
 */
class Screen {
public:
   using FrameFunction = std::function<void (float dt)>;
   using KeyFunction   = std::function<void (sf::Keyboard::Key key)>;

   // What the HUD shows.
   struct Hud {
      int   score {0};
      int   highScore {0};
      int   level {1};
      int   lives {0};
      float bar {-1};          // potion time left, 0 to 1; negative hides it
      std::string banner;      // message over the maze; empty hides it
   };

   // A menu over the game: a title, a line under it and a list of options.
   struct Menu {
      bool visible {false};
      std::string title;
      std::string subtitle;
      std::vector<std::string> options;
      int selected {0};
   };

   // The map must outlive the screen.
   explicit Screen (const TileMap &map);

   bool open (unsigned width, unsigned height, const std::string &title);
   void close () { m_window.close(); }

   // Called once per frame, with the time since the previous frame.
   void onFrame (FrameFunction f) { m_frame = std::move(f); }

   // Called for every key press, in order, without missing any.
   void onKey (KeyFunction f) { m_key = std::move(f); }

   // How the maze looks.
   void setBackgroundColor (sf::Color c) { m_background = c; }
   bool setWallImage  (const std::string &file);
   bool setFloorImage (const std::string &file);

   // Items: one int per tile (same size as the map). 0 = nothing; other
   // values are drawn with their image or, without one, as a circle.
   void setItems     (const std::vector<int> *grid) { m_items = grid; }
   void setItemColor (int item, sf::Color color, float radiusInTiles);
   bool setItemImage (int item, const std::string &file);

   // Characters, drawn in the order they were added.
   void add (NPC *npc) { m_npcs.push_back(npc); }

   /**
    * Loads the interface art from a folder with panel.png, button.png,
    * button_selected.png, bar_back.png and bar_green/yellow/red.png
    * (9-slice images), plus the text and title fonts.
    */
   bool loadInterface (const std::string &folder, const std::string &textFont,
                       const std::string &titleFont);
   bool setLifeIcon (const std::string &file);

   void setHud  (const Hud &hud)   { m_hud = hud; }
   void setMenu (const Menu &menu) { m_menu = menu; }

   // Main loop; returns when the window closes.
   void run ();

private:
   struct ItemStyle {
      sf::Color   color {255, 255, 255};
      float       radius {0.1f};
      sf::Texture image;
      bool        hasImage {false};
   };

   enum Align { LEFT_ALIGN, CENTER_ALIGN, RIGHT_ALIGN };

   const TileMap &m_map;
   sf::RenderWindow m_window;
   sf::View m_camera;       // world (maze)
   sf::View m_fixed;        // window pixels (HUD, menus)

   FrameFunction m_frame;
   KeyFunction   m_key;

   sf::Color   m_background {20, 16, 24};
   sf::Texture m_wallImage, m_floorImage;
   bool        m_hasWall {false}, m_hasFloor {false};

   const std::vector<int> *m_items {nullptr};
   std::vector<ItemStyle> m_styles;   // index = item value

   std::vector<NPC *> m_npcs;

   // Interface.
   bool        m_hasInterface {false};
   sf::Texture m_panel, m_button, m_buttonSelected;
   sf::Texture m_barBack, m_barGreen, m_barYellow, m_barRed;
   sf::Texture m_lifeIcon;
   sf::Font    m_font, m_titleFont;
   Hud  m_hud;
   Menu m_menu;

   void fitCamera (unsigned width, unsigned height);
   ItemStyle &style (int item);

   void drawMaze ();
   void drawItems ();
   void drawCharacters ();
   void drawHud ();
   void drawBanner ();
   void drawMenu ();

   // A 9-slice image stretched over a rectangle without stretching its
   // corners; "border" is the corner size in the image, "scale" its zoom.
   void drawPanel (const sf::Texture &texture, const sf::FloatRect &where,
                   float border, float scale);
   void drawText (const sf::Font &font, const std::string &text, unsigned size,
                  sf::Vector2f where, Align align, sf::Color color);
};
