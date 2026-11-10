#include "Screen.hpp"

#include <algorithm>

#include "Global.hpp"


// Stretches a sprite to cover exactly one tile.
static void fitToTile (sf::Sprite &sprite) {
   const sf::Texture *t = sprite.getTexture();
   if (t == nullptr || t->getSize().x == 0 || t->getSize().y == 0)
      return;
   sprite.setScale(TILE_SIZE / t->getSize().x, TILE_SIZE / t->getSize().y);
}


Screen::Screen (const TileMap &map) : m_map(map) {}

void Screen::setTopMargin (float tiles) {
   m_topMargin = tiles;
   if (m_window.isOpen())
      fitCamera(m_window.getSize().x, m_window.getSize().y);
}

bool Screen::open (unsigned width, unsigned height, const std::string &title) {
   m_window.create(sf::VideoMode(width, height), title);
   if (!m_window.isOpen())
      return false;

   m_window.setFramerateLimit(60);
   fitCamera(width, height);
   return true;
}

void Screen::fitCamera (unsigned width, unsigned height) {

   m_fixed = sf::View(sf::FloatRect(0, 0, (float) width, (float) height));

   // Visible world: the whole map plus the top margin.
   const float worldW = m_map.width() * TILE_SIZE;
   const float worldH = (m_map.height() + m_topMargin) * TILE_SIZE;
   m_camera = sf::View(sf::FloatRect(0, -m_topMargin * TILE_SIZE, worldW, worldH));

   // Same proportions as the world, centred in the window.
   const float windowRatio = (float) width / height;
   const float worldRatio  = worldW / worldH;

   if (windowRatio > worldRatio) {
      float w = worldRatio / windowRatio;
      m_camera.setViewport(sf::FloatRect((1 - w) / 2, 0, w, 1));
   } else {
      float h = windowRatio / worldRatio;
      m_camera.setViewport(sf::FloatRect(0, (1 - h) / 2, 1, h));
   }
}


//--------------------------------------------------------------
// Looks

bool Screen::setWallImage (const std::string &file) {
   m_hasWall = m_wallImage.loadFromFile(file);
   return m_hasWall;
}

bool Screen::setFloorImage (const std::string &file) {
   m_hasFloor = m_floorImage.loadFromFile(file);
   return m_hasFloor;
}

Screen::ItemStyle &Screen::style (int item) {
   if ((int) m_styles.size() <= item)
      m_styles.resize(item + 1);
   return m_styles[item];
}

void Screen::setItemColor (int item, sf::Color color, float radiusInTiles) {
   style(item).color  = color;
   style(item).radius = radiusInTiles;
}

bool Screen::setItemImage (int item, const std::string &file) {
   ItemStyle &s = style(item);
   s.hasImage = s.image.loadFromFile(file);
   return s.hasImage;
}

bool Screen::setHudFont (const std::string &file) {
   m_hasFont = m_font.loadFromFile(file);
   if (m_hasFont) {
      m_text.setFont(m_font);
      m_text.setCharacterSize(28);
      m_text.setFillColor(sf::Color::White);
      m_text.setPosition(16, 10);
   }
   return m_hasFont;
}

void Screen::setHudText (const std::string &utf8Text) {
   m_text.setString(sf::String::fromUtf8(utf8Text.begin(), utf8Text.end()));
}


//--------------------------------------------------------------
// Main loop

void Screen::run () {

   sf::Clock clock;

   while (m_window.isOpen()) {

      sf::Event event;
      while (m_window.pollEvent(event)) {
         if (event.type == sf::Event::Closed)
            m_window.close();
         else if (event.type == sf::Event::Resized)
            fitCamera(event.size.width, event.size.height);
         else if (event.type == sf::Event::KeyPressed && m_key)
            m_key(event.key.code);
      }

      float dt = clock.restart().asSeconds();
      if (m_frame)
         m_frame(dt);

      m_window.clear(m_background);

      m_window.setView(m_camera);
      drawMaze();
      drawItems();
      drawCharacters();

      m_window.setView(m_fixed);
      drawHud();

      m_window.display();
   }
}


//--------------------------------------------------------------
// Drawing

void Screen::drawMaze () {

   sf::Sprite wall(m_wallImage), floor(m_floorImage);
   fitToTile(wall);
   fitToTile(floor);

   sf::RectangleShape block(sf::Vector2f(TILE_SIZE, TILE_SIZE));
   block.setFillColor(m_wallColor);

   for (int row = 0; row < m_map.height(); ++row)
      for (int col = 0; col < m_map.width(); ++col) {
         sf::Vector2f where(col * TILE_SIZE, row * TILE_SIZE);

         if (m_map.isWall(col, row)) {
            if (m_hasWall) { wall.setPosition(where);  m_window.draw(wall);  }
            else           { block.setPosition(where); m_window.draw(block); }
         } else if (m_hasFloor) {
            floor.setPosition(where);
            m_window.draw(floor);
         }
      }
}

void Screen::drawItems () {

   if (m_items == nullptr)
      return;

   const int width = m_map.width();

   for (int row = 0; row < m_map.height(); ++row)
      for (int col = 0; col < width; ++col) {
         size_t i = (size_t) row * width + col;
         if (i >= m_items->size())
            return;

         int item = (*m_items)[i];
         if (item <= 0 || item >= (int) m_styles.size())
            continue;

         const ItemStyle &s = m_styles[item];

         if (s.hasImage) {
            sf::Sprite sprite(s.image);
            fitToTile(sprite);
            sprite.setPosition(col * TILE_SIZE, row * TILE_SIZE);
            m_window.draw(sprite);
         } else {
            float r = s.radius * TILE_SIZE;
            sf::CircleShape c(r);
            c.setOrigin(r, r);
            c.setFillColor(s.color);
            c.setPosition((col + 0.5f) * TILE_SIZE, (row + 0.5f) * TILE_SIZE);
            m_window.draw(c);
         }
      }
}

void Screen::drawCharacters () {
   for (NPC *npc: m_npcs) {
      if (npc == nullptr || !npc->active)
         continue;
      fitToTile(npc->m_sprite);
      npc->m_sprite.setPosition(npc->body.position.x * TILE_SIZE,
                                npc->body.position.y * TILE_SIZE);
      m_window.draw(npc->m_sprite);
   }
}

void Screen::drawHud () {

   if (!m_hasFont)
      return;

   const float barW = 240, barH = 16, gap = 10;
   const bool  hasBar = m_bar >= 0;

   sf::FloatRect t = m_text.getGlobalBounds();
   float w = hasBar ? std::max(t.width, barW) : t.width;
   float h = t.height + (hasBar ? gap + barH : 0);

   sf::RectangleShape panel(sf::Vector2f(w + 24, h + 20));
   panel.setPosition(t.left - 12, t.top - 10);
   panel.setFillColor(sf::Color(0, 0, 0, 170));
   m_window.draw(panel);
   m_window.draw(m_text);

   if (!hasBar)
      return;

   float top = t.top + t.height + gap;

   sf::RectangleShape empty(sf::Vector2f(barW, barH));
   empty.setPosition(t.left, top);
   empty.setFillColor(sf::Color(70, 70, 70));
   m_window.draw(empty);

   float fraction = std::min(m_bar, 1.0f);
   sf::RectangleShape full(sf::Vector2f(barW * fraction, barH));
   full.setPosition(t.left, top);
   full.setFillColor(fraction > 0.5f  ? sf::Color(60, 200, 80)
                   : fraction > 0.25f ? sf::Color(240, 200, 40)
                                      : sf::Color(220, 60, 50));
   m_window.draw(full);
}
