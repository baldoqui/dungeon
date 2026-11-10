#include "Jogo.hpp"

#include <algorithm>
#include <cmath>

#include "Collision2D.hpp"


//--------------------------------------------------------------
// Números das regras

const int   VIDAS_NO_COMECO        = 3;
const int   PONTOS_PONTINHO        = 10;
const int   PONTOS_POCAO           = 50;
const int   PONTOS_PRIMEIRO_MONSTRO = 200;

const float VELOCIDADE_HEROI       = 4.0f;   // tiles por segundo
const float VELOCIDADE_MONSTRO     = 3.6f;   // na fase 1
const float VELOCIDADE_MAXIMA      = 4.0f;   // monstros nunca passam disso
const float VELOCIDADE_VULNERAVEL  = 2.0f;
const float VELOCIDADE_SAINDO      = 2.0f;

const float DURACAO_DISPERSAO      = 7.0f;   // segundos
const float DURACAO_PERSEGUICAO    = 20.0f;
const float PAUSA_PRONTO           = 2.0f;
const float PAUSA_PEGO             = 1.5f;
const float PAUSA_FASE_COMPLETA    = 2.0f;
const float ESPERA_DEPOIS_DE_COMIDO = 3.0f;

// Folga das caixas de colisão: herói e monstro só se "pegam" quando
// os centros estão a menos de meio tile, nos dois eixos.
const float FOLGA = 0.25f;


//--------------------------------------------------------------
// Preparação

bool Jogo::prepara (const std::string &arquivoDoLabirinto) {

   if (!lab.carrega(arquivoDoLabirinto))
      return false;

   caminho = std::make_unique<PathFind>(lab.mapa.data(), lab.mapa.width(),
                                        lab.mapa.height());

   // Os cantos ficam um tile para dentro das bordas.
   const float direita  = lab.largura() - 2;
   const float embaixo  = lab.altura()  - 2;

   monstros[BRUTO].personalidade    = BRUTO;
   monstros[BRUTO].nome             = "Bruto";
   monstros[BRUTO].imagem           = "demonio.png";
   monstros[BRUTO].canto            = Vector2D(direita, 1);

   monstros[ESPREITA].personalidade = ESPREITA;
   monstros[ESPREITA].nome          = "Espreita";
   monstros[ESPREITA].imagem        = "morcego.png";
   monstros[ESPREITA].canto         = Vector2D(1, 1);

   monstros[CERCO].personalidade    = CERCO;
   monstros[CERCO].nome             = "Cerco";
   monstros[CERCO].imagem           = "aranha.png";
   monstros[CERCO].canto            = Vector2D(direita, embaixo);

   monstros[MEDROSO].personalidade  = MEDROSO;
   monstros[MEDROSO].nome           = "Medroso";
   monstros[MEDROSO].imagem         = "gosma.png";
   monstros[MEDROSO].canto          = Vector2D(1, embaixo);

   return true;
}

void Jogo::comecaJogo () {
   pontos = 0;
   vidas  = VIDAS_NO_COMECO;
   fase   = 1;
   lab.recolocaItens();
   comecaVida();
}

void Jogo::comecaVida () {

   heroi.colocaEm(lab.inicioHeroi);
   heroi.velocidade = VELOCIDADE_HEROI;
   desejo = ESQUERDA;

   // O Bruto já começa do lado de fora; os outros saem do curral um a um.
   const float esperas[NUMERO_DE_MONSTROS] = {0, 2, 5, 8};

   for (int i = 0; i < NUMERO_DE_MONSTROS; ++i) {
      Monstro &m = monstros[i];
      m.vulneravel = false;
      m.espera = esperas[i] / (1 + 0.2f * (fase - 1));

      if (i == BRUTO) {
         m.ator.colocaEm(lab.saidaDoCurral);
         m.estado = ANDANDO;
      } else {
         // Tiles do meio do curral.
         size_t meio = lab.curral.size() / 2;
         size_t tile = std::min(lab.curral.size() - 1, meio - 1 + (i - 1));
         m.ator.colocaEm(lab.curral[tile]);
         m.estado = NO_CURRAL;
      }
   }

   dispersao = true;
   tempoDoModo = DURACAO_DISPERSAO;
   tempoVulneravel = 0;
   comidosNaPocao = 0;

   etapa = PRONTO;
   pausa = PAUSA_PRONTO;
   atualizaCores();
}


//--------------------------------------------------------------
// Teclado

void Jogo::tecla (sf::Keyboard::Key k) {
   switch (k) {
      case sf::Keyboard::Left:  desejo = ESQUERDA; break;
      case sf::Keyboard::Right: desejo = DIREITA;  break;
      case sf::Keyboard::Up:    desejo = CIMA;     break;
      case sf::Keyboard::Down:  desejo = BAIXO;    break;
      case sf::Keyboard::R:
         if (etapa == FIM_DE_JOGO)
            comecaJogo();
         break;
      default:
         break;
   }
}


//--------------------------------------------------------------
// Um quadro

void Jogo::atualiza (float dt) {

   switch (etapa) {
      case PRONTO:
         pausa -= dt;
         if (pausa <= 0)
            etapa = JOGANDO;
         break;

      case JOGANDO:
         jogaUmQuadro(dt);
         break;

      case PEGO:
         pausa -= dt;
         if (pausa <= 0) {
            if (vidas <= 0)
               etapa = FIM_DE_JOGO;
            else
               comecaVida();
         }
         break;

      case FASE_COMPLETA:
         pausa -= dt;
         if (pausa <= 0) {
            ++fase;
            lab.recolocaItens();
            comecaVida();
         }
         break;

      case FIM_DE_JOGO:
         break;
   }

   recorde = std::max(recorde, pontos);
   atualizaCores();
}

void Jogo::jogaUmQuadro (float dt) {

   // Dispersão e perseguição se alternam; o relógio para durante a poção.
   if (tempoVulneravel > 0) {
      tempoVulneravel -= dt;
      if (tempoVulneravel <= 0)
         for (auto &m: monstros)
            m.vulneravel = false;
   } else {
      tempoDoModo -= dt;
      if (tempoDoModo <= 0) {
         dispersao = !dispersao;
         tempoDoModo = dispersao ? DURACAO_DISPERSAO : DURACAO_PERSEGUICAO;
      }
   }

   moveHeroi(dt);
   comeItem();

   if (lab.itensRestantes() == 0) {
      etapa = FASE_COMPLETA;
      pausa = PAUSA_FASE_COMPLETA;
      return;
   }

   for (auto &m: monstros)
      moveMonstro(m, dt);

   verificaEncontros();
}


//--------------------------------------------------------------
// Herói

void Jogo::moveHeroi (float dt) {

   Livre livre = [this](int col, int row) { return lab.livreParaHeroi(col, row); };

   // Meia-volta vale na hora, mesmo entre dois tiles.
   if (!heroi.dir.parado() && desejo == heroi.dir.contraria())
      heroi.dir = desejo;

   // No centro de um tile: vira para onde a seta mandou, se der;
   // senão segue reto (e para se bater na parede).
   Escolha escolhe = [&](const Ator &a) {
      if (livre(a.col() + desejo.dx, a.row() + desejo.dy))
         return desejo;
      return a.dir;
   };

   anda(heroi, dt, lab, livre, escolhe);
}

void Jogo::comeItem () {

   int col = heroi.col(), row = heroi.row();
   int item = lab.item(col, row);

   if (item == NADA)
      return;

   lab.tiraItem(col, row);

   if (item == PONTINHO)
      pontos += PONTOS_PONTINHO;

   if (item == POCAO) {
      pontos += PONTOS_POCAO;
      tempoVulneravel = duracaoDaPocao();
      comidosNaPocao = 0;
      for (auto &m: monstros) {
         m.vulneravel = true;
         // Ao ficarem vulneráveis, os que estão no labirinto dão meia-volta.
         if (m.estado == ANDANDO && !m.ator.dir.parado())
            m.ator.dir = m.ator.dir.contraria();
      }
   }
}


//--------------------------------------------------------------
// Monstros

float Jogo::velocidadeDosMonstros () const {
   return std::min(VELOCIDADE_MAXIMA, VELOCIDADE_MONSTRO * (1 + 0.05f * (fase - 1)));
}

float Jogo::duracaoDaPocao () const {
   // A poção dura menos a cada fase, até um mínimo de 2 segundos.
   return std::max(2.0f, 7.0f - (fase - 1));
}

void Jogo::moveMonstro (Monstro &m, float dt) {

   Livre livreNoLabirinto = [this](int col, int row) {
      return lab.livreParaHeroi(col, row);
   };
   Livre livreComCurral = [this](int col, int row) {
      return lab.livreParaMonstro(col, row);
   };

   switch (m.estado) {

      case NO_CURRAL:
         m.espera -= dt;
         if (m.espera <= 0)
            m.estado = SAINDO;
         return;

      case SAINDO: {
         m.ator.velocidade = VELOCIDADE_SAINDO;
         Escolha escolhe = [&](const Ator &a) {
            if (a.col() == (int) lab.saidaDoCurral.x &&
                a.row() == (int) lab.saidaDoCurral.y)
               return PARADO;
            return direcaoPara(a, lab.saidaDoCurral, *caminho, livreComCurral);
         };
         anda(m.ator, dt, lab, livreComCurral, escolhe);

         if (m.ator.noCentro() &&
             m.ator.col() == (int) lab.saidaDoCurral.x &&
             m.ator.row() == (int) lab.saidaDoCurral.y)
            m.estado = ANDANDO;
         return;
      }

      case ANDANDO: {
         m.ator.velocidade = m.vulneravel ? VELOCIDADE_VULNERAVEL
                                          : velocidadeDosMonstros();

         Situacao s;
         s.heroi     = Vector2D(heroi.col(), heroi.row());
         s.dirHeroi  = heroi.dir.parado() ? desejo : heroi.dir;
         s.bruto     = monstros[BRUTO].ator.npc.body.position;
         s.dispersao = dispersao;

         Escolha escolhe = [&](const Ator &a) {
            if (m.vulneravel)
               return direcaoDeFuga(a, s.heroi, livreNoLabirinto);
            Vector2D alvo = alvoValido(alvoDe(m, s), lab);
            return direcaoPara(a, alvo, *caminho, livreNoLabirinto);
         };
         anda(m.ator, dt, lab, livreNoLabirinto, escolhe);
         return;
      }
   }
}

void Jogo::verificaEncontros () {

   auto caixa = [](const Ator &a) {
      const Vector2D &p = a.npc.body.position;
      return AABB{ p + Vector2D(FOLGA, FOLGA), p + Vector2D(1 - FOLGA, 1 - FOLGA) };
   };

   for (auto &m: monstros) {
      if (m.estado == NO_CURRAL)
         continue;
      if (!caixa(heroi).intersects(caixa(m.ator)))
         continue;

      if (m.vulneravel) {
         // Comeu o monstro: pontos dobram a cada um, e ele volta ao curral.
         pontos += PONTOS_PRIMEIRO_MONSTRO << comidosNaPocao;
         ++comidosNaPocao;
         m.vulneravel = false;
         m.ator.colocaEm(lab.curral[lab.curral.size() / 2]);
         m.estado = NO_CURRAL;
         m.espera = ESPERA_DEPOIS_DE_COMIDO;
      } else {
         --vidas;
         etapa = PEGO;
         pausa = PAUSA_PEGO;
         return;
      }
   }
}


//--------------------------------------------------------------
// Aparência e placar

void Jogo::atualizaCores () {
   // Vulneráveis (desenhados como fantasma pelo Main.cpp) ficam azulados,
   // e piscam em branco nos últimos 2 segundos.
   bool piscando = tempoVulneravel < 2.0f && (int) (tempoVulneravel * 6) % 2 == 0;

   for (auto &m: monstros) {
      if (m.vulneravel && !piscando)
         m.ator.npc.m_sprite.setColor(sf::Color(130, 150, 255));
      else
         m.ator.npc.m_sprite.setColor(sf::Color::White);
   }
}

std::string Jogo::textoDoPlacar () const {

   std::string texto = "Pontos: " + std::to_string(pontos) +
                       "    Recorde: " + std::to_string(recorde) +
                       "\nFase: " + std::to_string(fase) +
                       "    Vidas: " + std::to_string(vidas);

   switch (etapa) {
      case PRONTO:        texto += "\nPreparar..."; break;
      case PEGO:          texto += "\nPego!"; break;
      case FASE_COMPLETA: texto += "\nFase completa!"; break;
      case FIM_DE_JOGO:   texto += "\nFim de jogo! Aperte R para recomeçar"; break;
      case JOGANDO:       break;
   }
   return texto;
}
