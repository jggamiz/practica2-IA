#include "tecnico.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>
#include <cstdlib>

using namespace std;

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

Action ComportamientoTecnico::think(Sensores sensores) {
  Action accion = IDLE;


  // Decisión del agente según el nivel
  switch (sensores.nivel) {
    case 0: accion = ComportamientoTecnicoNivel_0(sensores); break;
    case 1: accion = ComportamientoTecnicoNivel_1(sensores); break;
    case 2: accion = ComportamientoTecnicoNivel_2(sensores); break;
    // case 3: accion = ComportamientoTecnicoNivel_3(sensores); break;
    case 3: accion = ComportamientoTecnicoNivel_E(sensores); break;
    case 4: accion = ComportamientoTecnicoNivel_4(sensores); break;
    case 5: accion = ComportamientoTecnicoNivel_5(sensores); break;
    case 6: accion = ComportamientoTecnicoNivel_6(sensores); break;
  }

  return accion;
}


// --------------------------------------------------------------------------------
// Niveles iniciales (Comportamientos reactivos simples)
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 0

/**
 * @brief Determina si la casilla es viable por altura
 * @param casilla tipo de terreno
 * @param dif diferencia de altura entre casillas
 * @return 'P' si no es accesible por altura y casilla en otro caso
 */
char ViablePorAlturaT(char casilla, int dif) {
  if (abs(dif)<=1) return casilla;
  else return 'P';  
}


/**
 * @brief Determina la mejor opción entre las tres casillas que tiene delante
 * @param i terreno que tiene en la posición 1 de superficie (45 izq)
 * @param c terreno que tiene en la posición 2 de superficie (justo delante)
 * @param d terreno que tiene en la posición 3 de superficie (45 dch)
 * @param zap indica si el agente tiene las zapatillas
 * @return 2 si es mejor WALK, 1 para TURN_SL, 3 para TURN_SR y 0 si no hay nada interesante
 */
int VeoCasillaInteresanteT(char i, char c, char d, bool zap){
  if (c=='U') return 2;
  else if (i=='U') return 1;
  else if (d=='U') return 3;
  else if (!zap) {
    if (c=='D') return 2;
    else if (i=='D') return 1;
    else if (d=='D') return 3; 
  }
  if (c=='C') return 2;
  else if (i=='C') return 1;
  else if (d=='C') return 3;
  else if (zap) { // cuando el técnico tiene las zapatillas, 'B' es transitable
    if (c=='B') return 2;
    else if (i=='B') return 1;
    else if (d=='B') return 3;
  }
  
  return 0;
}


Action ComportamientoTecnico::ComportamientoTecnicoNivel_0(Sensores sensores) {
  Action accion = IDLE;
  // El comportamiento de seguir un camino hasta encontrar una plata de T. Residuos
  // Poner el valor de los sensores de visión en el mapa
  ActualizarMapa(sensores);

  // Actualización de variables de estado
  if (sensores.superficie[0]=='D') tiene_zapatillas = true;

  // Definición del comportamiento
  if (sensores.superficie[0]=='U') return IDLE; // ha llegado a una 'U'

  if (giro45Izq > 0) {
    giro45Izq--;
    accion = TURN_SL;
    last_action = accion;
    return accion; 
  }

  int current_cota = sensores.cota[0];
  char i = ViablePorAlturaT(sensores.superficie[1], sensores.cota[1]-current_cota);
  char c = ViablePorAlturaT(sensores.superficie[2], sensores.cota[2]-current_cota);
  char d = ViablePorAlturaT(sensores.superficie[3], sensores.cota[3]-current_cota);

  int pos = VeoCasillaInteresanteT(i, c, d, tiene_zapatillas);
  switch (pos) {
  case 2:
      accion = WALK;
      break;
    case 1:
      accion = TURN_SL;
      break;
    case 3:
      accion = TURN_SR;
      break;
    default:
      accion = TURN_SL;
      break;
  }

  // Devolver la siguiente acción a hacer
  last_action = accion;
  return accion;
}

/**
 * @brief Comprueba si una celda es de tipo camino transitable.
 * @param c Carácter que representa el tipo de superficie.
 * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
 */
bool ComportamientoTecnico::es_camino(unsigned char c) const {
  return (c == 'C' || c == 'D' || c == 'U');
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 1

/**
 * @brief Asigna una cómo de atractivo es un terreno dado
 * @param terreno tipo de terreno
 * @param zap indica si el agente tiene las zapatillas
 * @return Puntuación valorando el atractivo de dicho terreno
 */
int ValoraTerrenoT(char terreno, bool zap) {
  switch (terreno)
  {
    case 'C': case 'S':
      return 5; // Objetivo principal: Caminos y Senderos
      break;
    
    case 'X': case 'U': case 'D':
      return 4; // Otros elementos o casillas útiles
      break;
    
    case 'H':
      return 2; // Hierba: cuesta más energía
      break;
      
    case 'A':
      return 1; // Agua: muy costoso
      break;

    case 'B':
      if (zap)
        return 3; // Si tiene zapatillas, transitable
      else
        return 0; // Si no, no transitable
      break;

    case 'P': case 'M': default:
      return 0; // Precipicios y Muros (Intransitables)
      break;
  }
}

/**
 * @brief Determina la mejor opción para explorar evitando bucles
 * @param i terreno que tiene en la posición 1 de superficie (45 izq)
 * @param c terreno que tiene en la posición 2 de superficie (justo delante)
 * @param d terreno que tiene en la posición 3 de superficie (45 dch)
 * @param zap indica si el agente tiene las zapatillas
 * @return 2 (WALK), 1 (TURN_SL), 3 (TURN_SR) y 0 si está bloqueado
 */
int VeoCasillaExploracionT(char i, char c, char d, bool zap) {
  int vi = ValoraTerrenoT(i, zap);
  int vc = ValoraTerrenoT(c, zap);
  int vd = ValoraTerrenoT(d, zap);

  // Si estamos rodeados de obstáculos forzamos un giro
  if (vi==0 and vc==0 and vd==0) return 0;

  // La prioridad máaxima es ir de frente si es igual o mejor que los lados
  if (vc>=vi and vc>=vd and vc>0) return 2;

  // Si ir de frente es peor, elegimos el mejor de los lados
  if (vi>vd) return 1;
  if (vd>vi) return 3;

  // Si los dos lados son igual de buenos, elegimos uno al azar
  if (rand()%2 == 0) return 1;
  else return 3;
}


/**
 * @brief Comportamiento reactivo del técnico para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_1(Sensores sensores) {
  Action accion = IDLE;

  // Actualizamos el mapa con lo que vemos ahora mismo
  ActualizarMapa(sensores);

  // Actualizar variables de estado
  if (sensores.superficie[0]=='D') tiene_zapatillas = true;

  // Terminar maniobras pendientes
  if (giro45Izq>0) {
    giro45Izq--;
    accion = TURN_SL;
    last_action = accion;
    return accion;
  }

  // Esquivar técnico de frente
  if (sensores.agentes[2]=='t') {
    accion = TURN_SL;
    giro45Izq = 1; // Recordamos que estamos en mitad del giro
    last_action = accion;
    return accion;
  }

  // Evaluamos el terreno cercano
  int current_cota = sensores.cota[0];

  // Comprobamos que el técnico no esté en las diagonales
  char sup_i = (sensores.agentes[1] == 't') ? 'P' : sensores.superficie[1];
  char sup_c = sensores.superficie[2]; 
  char sup_d = (sensores.agentes[3] == 't') ? 'P' : sensores.superficie[3];

  // Comprobamos la viabilidad por altura (y por colisiones)
  char i = ViablePorAlturaT(sup_i, sensores.cota[1]-current_cota);
  char c = ViablePorAlturaT(sup_c, sensores.cota[2]-current_cota);
  char d = ViablePorAlturaT(sup_d, sensores.cota[3]-current_cota);

  // Toma de decisión
  int pos = VeoCasillaExploracionT(i, c, d, tiene_zapatillas);
  
  switch(pos) {
    case 2:
      accion = WALK;
      break;
    case 1:
      accion = TURN_SL; // Avanzamos en diagonal izquierda
      break;
    case 3:
      accion = TURN_SR; // Avanzamos en diagonal derecha
      break;
    default:
      // Si nos hemos metido en un rincón donde ni recto ni diagonales son transitables,
      // forzamos un giro de 90º para darnos la vuelta poco a poco.
      accion = TURN_SL;
      giro45Izq = 1; 
      break;
  }

  last_action = accion;
  return accion;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 2

/**
 * @brief Comportamiento del técnico para el Nivel 2.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_2(Sensores sensores) {
  return IDLE;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 3

/**
 * @brief Comportamiento del técnico para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_3(Sensores sensores) {
  return IDLE;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 4

/**
 * @brief Comportamiento del técnico para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_4(Sensores sensores) {
  return IDLE;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 5

/**
 * @brief Comportamiento del técnico para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_5(Sensores sensores) {
  return IDLE;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL 6

/**
 * @brief Comportamiento del técnico para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_6(Sensores sensores) {
  return IDLE;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// NIVEL ESPECIAL

list<Action> AvanzaASaltosDeCaballo() {
  list<Action> secuencia;

  secuencia.push_back(WALK);
  secuencia.push_back(WALK);
  secuencia.push_back(TURN_SR);
  secuencia.push_back(TURN_SR);
  secuencia.push_back(WALK);

  return secuencia;
}

// Funciones auxiliares

/**
 * @brief Calcula el estado resultante al avanzar una casilla en la dirección actual
 * @param st Estado actual del agente (ubicación y orientación).
 * @return Nuevo estado con la posición avanzada una casilla en la misma orientación.
 */
EstadoT NextCasillaTecnico(const EstadoT &st) {
  EstadoT next = st;

  switch(st.site.brujula) {
    case norte:
      next.site.f = st.site.f-1;
      break;
    case noreste:
      next.site.f = st.site.f-1;
      next.site.c = st.site.c+1;
      break;
    case este:
      next.site.c = st.site.c+1;
      break;
    case sureste:
      next.site.f = st.site.f+1;
      next.site.c = st.site.c+1;
      break;
    case sur:
      next.site.f = st.site.f+1;
      break;
    case suroeste:
      next.site.f = st.site.f+1;
      next.site.c = st.site.c-1;
      break;
    case oeste:
      next.site.c = st.site.c-1;
      break;
    case noroeste:
      next.site.f = st.site.f-1;
      next.site.c = st.site.c-1;
      break; 
  }

  return next;
}

/**
 * @brief Comprueba si la casilla situada delante del agente es transitable, 
 *        considerando el terreno y la diferencia de altura.
 * @param st Estado actual del agente.
 * @param terreno Matriz de tipos de terreno.
 * @param altura Matriz de cotas (alturas).
 * @return true si la casilla de delante y false en caso contrario.
 */
bool CasillaAccesibleTecnico(const EstadoT &st, const vector<vector<unsigned char>> &terreno,
                             const vector<vector<unsigned char>> &altura) {
  EstadoT next = NextCasillaTecnico(st);
  bool check1 = false, check2=false, check3=false;
  check1 = terreno[next.site.f][next.site.c]!='P' and terreno[next.site.f][next.site.c]!='M'; 
  check2 = terreno[next.site.f][next.site.c]!='B' or (terreno[next.site.f][next.site.c]=='B' and st.zapatillas);
  check3 = abs(altura[st.site.f][st.site.c]-altura[next.site.f][next.site.c]) <= 1;
  
  return check1 and check2 and check3;
}

/**
 * @brief Aplica una acción sobre un estado.
 * @param accion Acción a ejecutar (WALK, TURN_SR, TURN_SL).
 * @param st Estado actual del agente.
 * @param terreno Matriz de tipos de terreno.
 * @param altura Matriz de cotas.
 * @return EstadoT Nuevo estado tras aplicar la acción.
 */
EstadoT applyT(Action accion, const EstadoT &st, const vector<vector<unsigned char>> &terreno, 
               const vector<vector<unsigned char>> &altura){
  EstadoT next = st;
  switch(accion){
    case WALK:
      if (CasillaAccesibleTecnico(st, terreno, altura)){
        next = NextCasillaTecnico(st);
      }
      break;
    case TURN_SR:
      next.site.brujula = (Orientacion) ((next.site.brujula+1)%8);
      break;
    case TURN_SL:
      next.site.brujula = (Orientacion) ((next.site.brujula+7)%8);
      break;
  }

  return next;
}

/**
 * @brief Determina si un nodo está presente en una lista de nodos.
 * @param st  Nodo a buscar.
 * @param lista Lista de nodos donde se realiza la búsqueda.
 * @return true si el nodo st se encuentra en la lista y false en caso contrario.
 */
bool Find(const NodoT &st, const list<NodoT> &lista) {
  auto it = lista.begin();
  while (it!=lista.end() and !((*it)==st)) {
    it++;
  }

  return (it!=lista.end());
}

/**
 * @brief Primera aproximación del algoritmo de búsqueda en anchura
 * 
 * @param inicio Estado inicial de la búsqueda
 * @param final Estado final de la búsqueda
 * @param terreno Matriz que contiene la información del terreno
 * @param altura Matriz que contiene las alturas del mapa
 * 
 * @return La secuencia de acciones para llegar al estado final
 * @note Devuelve un plan vacío si no es posible encontrar un plan válido
 */
list<Action> ComportamientoTecnico::B_Anchura(const EstadoT &inicio, const EstadoT &final, 
                                              const vector<vector<unsigned char>> &terreno,
                                              const vector<vector<unsigned char>> &altura) {
  NodoT current_node;
  list<NodoT> frontier;
  list<NodoT> explored; 
  list<Action> path;

  current_node.estado = inicio;
  frontier.push_back(current_node);
  bool SolutionFound = (current_node.estado.site.f==final.site.f and current_node.estado.site.c==final.site.c);
  
  while (!SolutionFound and !frontier.empty()) {
    frontier.pop_front();
    explored.push_back(current_node);

    // Compruebo si estoy en una casilla que da las zapatillas
    if (terreno[current_node.estado.site.f][current_node.estado.site.c]=='D') current_node.estado.zapatillas = true;

    // Genero el hijo resultante de aplicar la acción WALK
    NodoT child_Walk = current_node;
    child_Walk.estado = applyT(WALK, current_node.estado, terreno, altura);
    if (child_Walk.estado.site.f==final.site.f and child_Walk.estado.site.c==final.site.c) {
      // El hijo generado es solución
      child_Walk.secuencia.push_back(WALK);
      current_node = child_Walk;
      SolutionFound = true;
    } else if (!Find(child_Walk, frontier) and !Find(child_Walk, explored)) {
      // Se mete en la lista de frontier después de añadir la acción WALK a la secuencia
      child_Walk.secuencia.push_back(WALK);
      frontier.push_back(child_Walk);
    }

    if (!SolutionFound) {
      // El hijo resultante de aplicar la acción TURN_SR
      NodoT child_TurnSR = current_node;
      child_TurnSR.estado = applyT(TURN_SR, current_node.estado, terreno, altura);
      if (!Find(child_TurnSR, frontier) and !Find(child_TurnSR, explored)) {
        // Se mete en la lista de frontier después de añadir la acción TURN_SR a la secuencia
        child_TurnSR.secuencia.push_back(TURN_SR);
        frontier.push_back(child_TurnSR);
      }

      // El hijo resultante de aplicar la acción TURN_SL
      NodoT child_TurnSL = current_node;
      child_TurnSL.estado = applyT(TURN_SL, current_node.estado, terreno, altura);
      if (!Find(child_TurnSL, frontier) and !Find(child_TurnSL, explored)) {
        // Se mete en la lista de frontier después de añadir la acción TURN_SL a la secuencia
        child_TurnSL.secuencia.push_back(TURN_SL);
        frontier.push_back(child_TurnSL);
      }
    }

    // Paso a evaluar el siguient nodo en la lista frontier
    if (!SolutionFound and !frontier.empty()) {
      current_node = frontier.front();
      SolutionFound = (current_node.estado.site.f==final.site.f and current_node.estado.site.c==final.site.c);
    }
  }

  // Devuelvo el camino encontrado
  if (SolutionFound) path=current_node.secuencia;
  return path;
}

/**
 * @brief Segunda aproximación del algoritmo de búsqueda en anchura
 * 
 * @param inicio Estado inicial de la búsqueda
 * @param final Estado final de la búsqueda
 * @param terreno Matriz que contiene la información del terreno
 * @param altura Matriz que contiene las alturas del mapa
 * 
 * @return La secuencia de acciones para llegar al estado final
 * @note Devuelve un plan vacío si no es posible encontrar un plan válido
 * @note Explored pasa a ser implementado mediante un "set" en vez de un "list"
 */
list<Action> ComportamientoTecnico::B_Anchura_V2(const EstadoT &inicio, const EstadoT &final, 
                                              const vector<vector<unsigned char>> &terreno,
                                              const vector<vector<unsigned char>> &altura) {
  NodoT current_node;
  list<NodoT> frontier;
  set<NodoT> explored; 
  list<Action> path;

  current_node.estado = inicio;
  frontier.push_back(current_node);
  bool SolutionFound = (current_node.estado.site.f==final.site.f and current_node.estado.site.c==final.site.c);
  
  while (!SolutionFound and !frontier.empty()) {
    frontier.pop_front();
    explored.insert(current_node);

    // Compruebo si estoy en una casilla que da las zapatillas
    if (terreno[current_node.estado.site.f][current_node.estado.site.c]=='D') current_node.estado.zapatillas = true;

    // Genero el hijo resultante de aplicar la acción WALK
    NodoT child_Walk = current_node;
    child_Walk.estado = applyT(WALK, current_node.estado, terreno, altura);
    if (child_Walk.estado.site.f==final.site.f and child_Walk.estado.site.c==final.site.c) {
      // El hijo generado es solución
      child_Walk.secuencia.push_back(WALK);
      current_node = child_Walk;
      SolutionFound = true;
    } else if (explored.find(child_Walk)==explored.end()) {
      // Se mete en la lista de frontier después de añadir la acción WALK a la secuencia
      child_Walk.secuencia.push_back(WALK);
      frontier.push_back(child_Walk);
    }

    if (!SolutionFound) {
      // El hijo resultante de aplicar la acción TURN_SR
      NodoT child_TurnSR = current_node;
      child_TurnSR.estado = applyT(TURN_SR, current_node.estado, terreno, altura);
      if (explored.find(child_TurnSR)==explored.end()) {
        // Se mete en la lista de frontier después de añadir la acción TURN_SR a la secuencia
        child_TurnSR.secuencia.push_back(TURN_SR);
        frontier.push_back(child_TurnSR);
      }

      // El hijo resultante de aplicar la acción TURN_SL
      NodoT child_TurnSL = current_node;
      child_TurnSL.estado = applyT(TURN_SL, current_node.estado, terreno, altura);
      if (explored.find(child_TurnSL)==explored.end()) {
        // Se mete en la lista de frontier después de añadir la acción TURN_SL a la secuencia
        child_TurnSL.secuencia.push_back(TURN_SL);
        frontier.push_back(child_TurnSL);
      }
    }

    // Paso a evaluar el siguient nodo en la lista frontier
    if (!SolutionFound and !frontier.empty()) {
      current_node = frontier.front();
      while (explored.find(current_node)!=explored.end() and !frontier.empty()) {
        frontier.pop_front();
        current_node = frontier.front();
      }
    }
  }

  // Devuelvo el camino encontrado
  if (SolutionFound) path=current_node.secuencia;
  return path;
}

/**
 * @brief Comportamiento del técnico para el Nivel Especial.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoTecnico::ComportamientoTecnicoNivel_E(Sensores sensores) {
  Action accion = IDLE;

  if (!hayPlan) {
    // Invocar al método de búsqueda
    EstadoT inicio, fin;
    inicio.site.f = sensores.posF;
    inicio.site.c = sensores.posC;
    inicio.site.brujula = sensores.rumbo;
    inicio.zapatillas = tiene_zapatillas;
    fin.site.f = sensores.BelPosF;
    fin.site.c = sensores.BelPosC;

    // plan = AvanzaASaltosDeCaballo();
    // plan = B_Anchura(inicio, fin, mapaResultado, mapaCotas);
    plan = B_Anchura_V2(inicio, fin, mapaResultado, mapaCotas);
    VisualizaPlan(inicio.site, plan);

    hayPlan = (plan.size()!=0);
  }

  if (hayPlan and plan.size()>0) {
    accion = plan.front();
    plan.pop_front();
  }

  if (plan.size()==0) hayPlan = false;

  return accion;
}
// --------------------------------------------------------------------------------


// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

/**
 * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
 * @param sensores Datos actuales de los sensores.
 */
void ComportamientoTecnico::ActualizarMapa(Sensores sensores) {
  mapaResultado[sensores.posF][sensores.posC] = sensores.superficie[0];
  mapaCotas[sensores.posF][sensores.posC] = sensores.cota[0];

  int pos = 1;
  switch (sensores.rumbo) {
    case norte:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF - j][sensores.posC + i] = sensores.superficie[pos];
          mapaCotas[sensores.posF - j][sensores.posC + i] = sensores.cota[pos++];
        }
      break;
    case noreste:
      mapaResultado[sensores.posF - 1][sensores.posC] = sensores.superficie[1];
      mapaCotas[sensores.posF - 1][sensores.posC] = sensores.cota[1];
      mapaResultado[sensores.posF - 1][sensores.posC + 1] = sensores.superficie[2];
      mapaCotas[sensores.posF - 1][sensores.posC + 1] = sensores.cota[2];
      mapaResultado[sensores.posF][sensores.posC + 1] = sensores.superficie[3];
      mapaCotas[sensores.posF][sensores.posC + 1] = sensores.cota[3];
      mapaResultado[sensores.posF - 2][sensores.posC] = sensores.superficie[4];
      mapaCotas[sensores.posF - 2][sensores.posC] = sensores.cota[4];
      mapaResultado[sensores.posF - 2][sensores.posC + 1] = sensores.superficie[5];
      mapaCotas[sensores.posF - 2][sensores.posC + 1] = sensores.cota[5];
      mapaResultado[sensores.posF - 2][sensores.posC + 2] = sensores.superficie[6];
      mapaCotas[sensores.posF - 2][sensores.posC + 2] = sensores.cota[6];
      mapaResultado[sensores.posF - 1][sensores.posC + 2] = sensores.superficie[7];
      mapaCotas[sensores.posF - 1][sensores.posC + 2] = sensores.cota[7];
      mapaResultado[sensores.posF][sensores.posC + 2] = sensores.superficie[8];
      mapaCotas[sensores.posF][sensores.posC + 2] = sensores.cota[8];
      mapaResultado[sensores.posF - 3][sensores.posC] = sensores.superficie[9];
      mapaCotas[sensores.posF - 3][sensores.posC] = sensores.cota[9];
      mapaResultado[sensores.posF - 3][sensores.posC + 1] = sensores.superficie[10];
      mapaCotas[sensores.posF - 3][sensores.posC + 1] = sensores.cota[10];
      mapaResultado[sensores.posF - 3][sensores.posC + 2] = sensores.superficie[11];
      mapaCotas[sensores.posF - 3][sensores.posC + 2] = sensores.cota[11];
      mapaResultado[sensores.posF - 3][sensores.posC + 3] = sensores.superficie[12];
      mapaCotas[sensores.posF - 3][sensores.posC + 3] = sensores.cota[12];
      mapaResultado[sensores.posF - 2][sensores.posC + 3] = sensores.superficie[13];
      mapaCotas[sensores.posF - 2][sensores.posC + 3] = sensores.cota[13];
      mapaResultado[sensores.posF - 1][sensores.posC + 3] = sensores.superficie[14];
      mapaCotas[sensores.posF - 1][sensores.posC + 3] = sensores.cota[14];
      mapaResultado[sensores.posF][sensores.posC + 3] = sensores.superficie[15];
      mapaCotas[sensores.posF][sensores.posC + 3] = sensores.cota[15];
      break;
    case este:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF + i][sensores.posC + j] = sensores.superficie[pos];
          mapaCotas[sensores.posF + i][sensores.posC + j] = sensores.cota[pos++];
        }
      break;
    case sureste:
      mapaResultado[sensores.posF][sensores.posC + 1] = sensores.superficie[1];
      mapaCotas[sensores.posF][sensores.posC + 1] = sensores.cota[1];
      mapaResultado[sensores.posF + 1][sensores.posC + 1] = sensores.superficie[2];
      mapaCotas[sensores.posF + 1][sensores.posC + 1] = sensores.cota[2];
      mapaResultado[sensores.posF + 1][sensores.posC] = sensores.superficie[3];
      mapaCotas[sensores.posF + 1][sensores.posC] = sensores.cota[3];
      mapaResultado[sensores.posF][sensores.posC + 2] = sensores.superficie[4];
      mapaCotas[sensores.posF][sensores.posC + 2] = sensores.cota[4];
      mapaResultado[sensores.posF + 1][sensores.posC + 2] = sensores.superficie[5];
      mapaCotas[sensores.posF + 1][sensores.posC + 2] = sensores.cota[5];
      mapaResultado[sensores.posF + 2][sensores.posC + 2] = sensores.superficie[6];
      mapaCotas[sensores.posF + 2][sensores.posC + 2] = sensores.cota[6];
      mapaResultado[sensores.posF + 2][sensores.posC + 1] = sensores.superficie[7];
      mapaCotas[sensores.posF + 2][sensores.posC + 1] = sensores.cota[7];
      mapaResultado[sensores.posF + 2][sensores.posC] = sensores.superficie[8];
      mapaCotas[sensores.posF + 2][sensores.posC] = sensores.cota[8];
      mapaResultado[sensores.posF][sensores.posC + 3] = sensores.superficie[9];
      mapaCotas[sensores.posF][sensores.posC + 3] = sensores.cota[9];
      mapaResultado[sensores.posF + 1][sensores.posC + 3] = sensores.superficie[10];
      mapaCotas[sensores.posF + 1][sensores.posC + 3] = sensores.cota[10];
      mapaResultado[sensores.posF + 2][sensores.posC + 3] = sensores.superficie[11];
      mapaCotas[sensores.posF + 2][sensores.posC + 3] = sensores.cota[11];
      mapaResultado[sensores.posF + 3][sensores.posC + 3] = sensores.superficie[12];
      mapaCotas[sensores.posF + 3][sensores.posC + 3] = sensores.cota[12];
      mapaResultado[sensores.posF + 3][sensores.posC + 2] = sensores.superficie[13];
      mapaCotas[sensores.posF + 3][sensores.posC + 2] = sensores.cota[13];
      mapaResultado[sensores.posF + 3][sensores.posC + 1] = sensores.superficie[14];
      mapaCotas[sensores.posF + 3][sensores.posC + 1] = sensores.cota[14];
      mapaResultado[sensores.posF + 3][sensores.posC] = sensores.superficie[15];
      mapaCotas[sensores.posF + 3][sensores.posC] = sensores.cota[15];
      break;
    case sur:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF + j][sensores.posC - i] = sensores.superficie[pos];
          mapaCotas[sensores.posF + j][sensores.posC - i] = sensores.cota[pos++];
        }
      break;
    case suroeste:
      mapaResultado[sensores.posF + 1][sensores.posC] = sensores.superficie[1];
      mapaCotas[sensores.posF + 1][sensores.posC] = sensores.cota[1];
      mapaResultado[sensores.posF + 1][sensores.posC - 1] = sensores.superficie[2];
      mapaCotas[sensores.posF + 1][sensores.posC - 1] = sensores.cota[2];
      mapaResultado[sensores.posF][sensores.posC - 1] = sensores.superficie[3];
      mapaCotas[sensores.posF][sensores.posC - 1] = sensores.cota[3];
      mapaResultado[sensores.posF + 2][sensores.posC] = sensores.superficie[4];
      mapaCotas[sensores.posF + 2][sensores.posC] = sensores.cota[4];
      mapaResultado[sensores.posF + 2][sensores.posC - 1] = sensores.superficie[5];
      mapaCotas[sensores.posF + 2][sensores.posC - 1] = sensores.cota[5];
      mapaResultado[sensores.posF + 2][sensores.posC - 2] = sensores.superficie[6];
      mapaCotas[sensores.posF + 2][sensores.posC - 2] = sensores.cota[6];
      mapaResultado[sensores.posF + 1][sensores.posC - 2] = sensores.superficie[7];
      mapaCotas[sensores.posF + 1][sensores.posC - 2] = sensores.cota[7];
      mapaResultado[sensores.posF][sensores.posC - 2] = sensores.superficie[8];
      mapaCotas[sensores.posF][sensores.posC - 2] = sensores.cota[8];
      mapaResultado[sensores.posF + 3][sensores.posC] = sensores.superficie[9];
      mapaCotas[sensores.posF + 3][sensores.posC] = sensores.cota[9];
      mapaResultado[sensores.posF + 3][sensores.posC - 1] = sensores.superficie[10];
      mapaCotas[sensores.posF + 3][sensores.posC - 1] = sensores.cota[10];
      mapaResultado[sensores.posF + 3][sensores.posC - 2] = sensores.superficie[11];
      mapaCotas[sensores.posF + 3][sensores.posC - 2] = sensores.cota[11];
      mapaResultado[sensores.posF + 3][sensores.posC - 3] = sensores.superficie[12];
      mapaCotas[sensores.posF + 3][sensores.posC - 3] = sensores.cota[12];
      mapaResultado[sensores.posF + 2][sensores.posC - 3] = sensores.superficie[13];
      mapaCotas[sensores.posF + 2][sensores.posC - 3] = sensores.cota[13];
      mapaResultado[sensores.posF + 1][sensores.posC - 3] = sensores.superficie[14];
      mapaCotas[sensores.posF + 1][sensores.posC - 3] = sensores.cota[14];
      mapaResultado[sensores.posF][sensores.posC - 3] = sensores.superficie[15];
      mapaCotas[sensores.posF][sensores.posC - 3] = sensores.cota[15];
      break;
    case oeste:
      for (int j = 1; j < 4; j++)
        for (int i = -j; i <= j; i++) {
          mapaResultado[sensores.posF - i][sensores.posC - j] = sensores.superficie[pos];
          mapaCotas[sensores.posF - i][sensores.posC - j] = sensores.cota[pos++];
        }
      break;
    case noroeste:
      mapaResultado[sensores.posF][sensores.posC - 1] = sensores.superficie[1];
      mapaCotas[sensores.posF][sensores.posC - 1] = sensores.cota[1];
      mapaResultado[sensores.posF - 1][sensores.posC - 1] = sensores.superficie[2];
      mapaCotas[sensores.posF - 1][sensores.posC - 1] = sensores.cota[2];
      mapaResultado[sensores.posF - 1][sensores.posC] = sensores.superficie[3];
      mapaCotas[sensores.posF - 1][sensores.posC] = sensores.cota[3];
      mapaResultado[sensores.posF][sensores.posC - 2] = sensores.superficie[4];
      mapaCotas[sensores.posF][sensores.posC - 2] = sensores.cota[4];
      mapaResultado[sensores.posF - 1][sensores.posC - 2] = sensores.superficie[5];
      mapaCotas[sensores.posF - 1][sensores.posC - 2] = sensores.cota[5];
      mapaResultado[sensores.posF - 2][sensores.posC - 2] = sensores.superficie[6];
      mapaCotas[sensores.posF - 2][sensores.posC - 2] = sensores.cota[6];
      mapaResultado[sensores.posF - 2][sensores.posC - 1] = sensores.superficie[7];
      mapaCotas[sensores.posF - 2][sensores.posC - 1] = sensores.cota[7];
      mapaResultado[sensores.posF - 2][sensores.posC] = sensores.superficie[8];
      mapaCotas[sensores.posF - 2][sensores.posC] = sensores.cota[8];
      mapaResultado[sensores.posF][sensores.posC - 3] = sensores.superficie[9];
      mapaCotas[sensores.posF][sensores.posC - 3] = sensores.cota[9];
      mapaResultado[sensores.posF - 1][sensores.posC - 3] = sensores.superficie[10];
      mapaCotas[sensores.posF - 1][sensores.posC - 3] = sensores.cota[10];
      mapaResultado[sensores.posF - 2][sensores.posC - 3] = sensores.superficie[11];
      mapaCotas[sensores.posF - 2][sensores.posC - 3] = sensores.cota[11];
      mapaResultado[sensores.posF - 3][sensores.posC - 3] = sensores.superficie[12];
      mapaCotas[sensores.posF - 3][sensores.posC - 3] = sensores.cota[12];
      mapaResultado[sensores.posF - 3][sensores.posC - 2] = sensores.superficie[13];
      mapaCotas[sensores.posF - 3][sensores.posC - 2] = sensores.cota[13];
      mapaResultado[sensores.posF - 3][sensores.posC - 1] = sensores.superficie[14];
      mapaCotas[sensores.posF - 3][sensores.posC - 1] = sensores.cota[14];
      mapaResultado[sensores.posF - 3][sensores.posC] = sensores.superficie[15];
      mapaCotas[sensores.posF - 3][sensores.posC] = sensores.cota[15];
      break;
  }
}



/**
 * @brief Determina si una casilla es transitable para el técnico.
 * En esta práctica, si el técnico tiene zapatillas, el bosque ('B') es transitable.
 * @param f Fila de la casilla.
 * @param c Columna de la casilla.
 * @param tieneZapatillas Indica si el agente posee las zapatillas.
 * @return true si la casilla es transitable.
 */
bool ComportamientoTecnico::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas) {
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size()) return false;
  return es_camino(mapaResultado[f][c]);  // Solo 'C', 'S', 'D', 'U' son transitables en Nivel 0
}

/**
 * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
 * Para el técnico: desnivel máximo siempre 1.
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return true si el desnivel con la casilla de delante es admisible.
 */
bool ComportamientoTecnico::EsAccesiblePorAltura(const ubicacion &actual) {
  ubicacion del = Delante(actual);
  if (del.f < 0 || del.f >= mapaCotas.size() || del.c < 0 || del.c >= mapaCotas[0].size()) return false;
  int desnivel = abs(mapaCotas[del.f][del.c] - mapaCotas[actual.f][actual.c]);
  if (desnivel > 1) return false;
  return true;
}

/**
 * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
 * Calcula la casilla frontal según la orientación actual (8 direcciones).
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return Estado con la fila y columna de la casilla de enfrente.
 */
ubicacion ComportamientoTecnico::Delante(const ubicacion &actual) const {
  ubicacion delante = actual;
  switch (actual.brujula) {
    case 0: delante.f--; break;                        // norte
    case 1: delante.f--; delante.c++; break;     // noreste
    case 2: delante.c++; break;                     // este
    case 3: delante.f++; delante.c++; break;     // sureste
    case 4: delante.f++; break;                        // sur
    case 5: delante.f++; delante.c--; break;     // suroeste
    case 6: delante.c--; break;                     // oeste
    case 7: delante.f--; delante.c--; break;     // noroeste
  }
  return delante;
}


/**
 * @brief Imprime por consola la secuencia de acciones de un plan.
 *
 * @param plan  Lista de acciones del plan.
 */
void ComportamientoTecnico::PintaPlan(const list<Action> &plan)
{
  auto it = plan.begin();
  while (it != plan.end())
  {
    if (*it == WALK)
    {
      cout << "W ";
    }
    else if (*it == JUMP)
    {
      cout << "J ";
    }
    else if (*it == TURN_SR)
    {
      cout << "r ";
    }
    else if (*it == TURN_SL)
    {
      cout << "l ";
    }
    else if (*it == COME)
    {
      cout << "C ";
    }
    else if (*it == IDLE)
    {
      cout << "I ";
    }
    else
    {
      cout << "-_ ";
    }
    it++;
  }
  cout << "( longitud " << plan.size() << ")" << endl;
}



/**
 * @brief Convierte un plan de acciones en una lista de casillas para
 *        su visualización en el mapa 2D.
 *
 * @param st    Estado de partida.
 * @param plan  Lista de acciones del plan.
 */
void ComportamientoTecnico::VisualizaPlan(const ubicacion &st,
                                            const list<Action> &plan)
{
   listaPlanCasillas.clear();
  ubicacion cst = st;

  listaPlanCasillas.push_back({cst.f, cst.c, WALK});
  auto it = plan.begin();
  while (it != plan.end())
  {

    switch (*it)
    {
    case JUMP:
      switch (cst.brujula)
      {
      case 0:
        cst.f--;
        break;
      case 1:
        cst.f--;
        cst.c++;
        break;
      case 2:
        cst.c++;
        break;
      case 3:
        cst.f++;
        cst.c++;
        break;
      case 4:
        cst.f++;
        break;
      case 5:
        cst.f++;
        cst.c--;
        break;
      case 6:
        cst.c--;
        break;
      case 7:
        cst.f--;
        cst.c--;
        break;
      }
      if (cst.f >= 0 && cst.f < mapaResultado.size() &&
          cst.c >= 0 && cst.c < mapaResultado[0].size())
        listaPlanCasillas.push_back({cst.f, cst.c, JUMP});
    case WALK:
      switch (cst.brujula)
      {
      case 0:
        cst.f--;
        break;
      case 1:
        cst.f--;
        cst.c++;
        break;
      case 2:
        cst.c++;
        break;
      case 3:
        cst.f++;
        cst.c++;
        break;
      case 4:
        cst.f++;
        break;
      case 5:
        cst.f++;
        cst.c--;
        break;
      case 6:
        cst.c--;
        break;
      case 7:
        cst.f--;
        cst.c--;
        break;
      }
      if (cst.f >= 0 && cst.f < mapaResultado.size() &&
          cst.c >= 0 && cst.c < mapaResultado[0].size())
        listaPlanCasillas.push_back({cst.f, cst.c, WALK});
      break;
    case TURN_SR:
      cst.brujula = (Orientacion) (( (int) cst.brujula + 1) % 8);
      break;
    case TURN_SL:
      cst.brujula = (Orientacion) (( (int) cst.brujula + 7) % 8);
      break;
    }
    it++;
  }
}


