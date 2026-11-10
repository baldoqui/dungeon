#include "Maze.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "Global.hpp"


bool Maze::load (const std::string &file) {

   std::ifstream in(file);
   if (!in)
      return false;

   m_rows.clear();
   std::string line;
   while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r')
         line.pop_back();
      if (!line.empty())
         m_rows.push_back(line);
   }

   if (m_rows.empty())
      return false;

   m_height = (int) m_rows.size();
   m_width  = (int) m_rows[0].size();
   for (const auto &r: m_rows)
      if ((int) r.size() != m_width)
         return false;

   // Items, hero start and pen.
   m_startItems.assign(m_width * m_height, NO_ITEM);
   pen.clear();

   for (int row = 0; row < m_height; ++row)
      for (int col = 0; col < m_width; ++col) {
         char c = at(col, row);
         if (c == '.') m_startItems[row * m_width + col] = COIN;
         if (c == 'o') m_startItems[row * m_width + col] = POTION;
         if (c == 'H') heroStart = Vector2D(col, row);
         if (c == 'P') pen.push_back(Vector2D(col, row));
         if (c == '-') penExit = Vector2D(col, row - 1);
      }

   resetItems();

   // TileMap can only read CSV (TileMap::loadCSV), so the walls go
   // through a temporary CSV file.
   std::filesystem::path csv =
      std::filesystem::temp_directory_path() / "dungeon-maze.csv";
   {
      std::ofstream out(csv);
      for (int row = 0; row < m_height; ++row) {
         for (int col = 0; col < m_width; ++col) {
            if (col > 0) out << ',';
            out << (at(col, row) == '#' ? WALL_TILE : EMPTY_TILE);
         }
         out << '\n';
      }
   }
   bool ok = map.loadCSV(csv.string());
   std::remove(csv.string().c_str());

   return ok;
}

char Maze::at (int col, int row) const {
   return m_rows[row][col];
}

int Maze::wrapColumn (int col) const {
   return ((col % m_width) + m_width) % m_width;
}

bool Maze::isOpenForMonster (int col, int row) const {
   if (row < 0 || row >= m_height)
      return false;
   return at(wrapColumn(col), row) != '#';
}

bool Maze::isOpenForHero (int col, int row) const {
   if (!isOpenForMonster(col, row))
      return false;
   char c = at(wrapColumn(col), row);
   return c != 'P' && c != '-';
}

bool Maze::isInPen (int col, int row) const {
   if (row < 0 || row >= m_height)
      return false;
   char c = at(wrapColumn(col), row);
   return c == 'P' || c == '-';
}

int Maze::item (int col, int row) const {
   if (row < 0 || row >= m_height)
      return NO_ITEM;
   return m_items[row * m_width + wrapColumn(col)];
}

void Maze::removeItem (int col, int row) {
   if (item(col, row) == NO_ITEM)
      return;
   m_items[row * m_width + wrapColumn(col)] = NO_ITEM;
   --m_itemsLeft;
}

void Maze::resetItems () {
   m_items = m_startItems;
   m_itemsLeft = (int) std::count_if(m_items.begin(), m_items.end(),
                                     [](int i) { return i != NO_ITEM; });
}
