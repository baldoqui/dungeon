#include "Labirinto.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "Global.hpp"


bool Labirinto::carrega (const std::string &arquivo) {

   std::ifstream in(arquivo);
   if (!in)
      return false;

   m_linhas.clear();
   std::string linha;
   while (std::getline(in, linha)) {
      if (!linha.empty() && linha.back() == '\r')
         linha.pop_back();
      if (!linha.empty())
         m_linhas.push_back(linha);
   }

   if (m_linhas.empty())
      return false;

   m_altura  = (int) m_linhas.size();
   m_largura = (int) m_linhas[0].size();
   for (const auto &l: m_linhas)
      if ((int) l.size() != m_largura)
         return false;

   // Itens, início do herói e curral.
   m_itensDoComeco.assign(m_largura * m_altura, NADA);
   curral.clear();

   for (int row = 0; row < m_altura; ++row)
      for (int col = 0; col < m_largura; ++col) {
         char c = letra(col, row);
         if (c == '.') m_itensDoComeco[row * m_largura + col] = PONTINHO;
         if (c == 'o') m_itensDoComeco[row * m_largura + col] = POCAO;
         if (c == 'H') inicioHeroi = Vector2D(col, row);
         if (c == 'P') curral.push_back(Vector2D(col, row));
         if (c == '-') saidaDoCurral = Vector2D(col, row - 1);
      }

   recolocaItens();

   // O TileMap só sabe ler CSV (TileMap::loadCSV), então as paredes
   // passam por um CSV temporário.
   std::filesystem::path csv =
      std::filesystem::temp_directory_path() / "dungeon-labirinto.csv";
   {
      std::ofstream out(csv);
      for (int row = 0; row < m_altura; ++row) {
         for (int col = 0; col < m_largura; ++col) {
            if (col > 0) out << ',';
            out << (letra(col, row) == '#' ? WALL_TILE : EMPTY_TILE);
         }
         out << '\n';
      }
   }
   bool ok = mapa.loadCSV(csv.string());
   std::remove(csv.string().c_str());

   return ok;
}

char Labirinto::letra (int col, int row) const {
   return m_linhas[row][col];
}

int Labirinto::dentroDoMapa (int col) const {
   return ((col % m_largura) + m_largura) % m_largura;
}

bool Labirinto::livreParaMonstro (int col, int row) const {
   if (row < 0 || row >= m_altura)
      return false;
   return letra(dentroDoMapa(col), row) != '#';
}

bool Labirinto::livreParaHeroi (int col, int row) const {
   if (!livreParaMonstro(col, row))
      return false;
   char c = letra(dentroDoMapa(col), row);
   return c != 'P' && c != '-';
}

bool Labirinto::noCurral (int col, int row) const {
   if (row < 0 || row >= m_altura)
      return false;
   char c = letra(dentroDoMapa(col), row);
   return c == 'P' || c == '-';
}

int Labirinto::item (int col, int row) const {
   if (row < 0 || row >= m_altura)
      return NADA;
   return m_itens[row * m_largura + dentroDoMapa(col)];
}

void Labirinto::tiraItem (int col, int row) {
   if (item(col, row) == NADA)
      return;
   m_itens[row * m_largura + dentroDoMapa(col)] = NADA;
   --m_restantes;
}

void Labirinto::recolocaItens () {
   m_itens = m_itensDoComeco;
   m_restantes = (int) std::count_if(m_itens.begin(), m_itens.end(),
                                     [](int i) { return i != NADA; });
}
