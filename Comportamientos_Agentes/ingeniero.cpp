#include "ingeniero.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <queue>
#include <set>
#include <map>
#include <utility>

using namespace std;

// =========================================================================
// ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
// =========================================================================

Action ComportamientoIngeniero::think(Sensores sensores)
{
  Action accion = IDLE;

  // Decisión del agente según el nivel
  switch (sensores.nivel)
  {
  case 0: accion = ComportamientoIngenieroNivel_0(sensores); break;
  case 1: accion = ComportamientoIngenieroNivel_1(sensores); break;
  case 2: accion = ComportamientoIngenieroNivel_2(sensores); break;
  case 3: accion = ComportamientoIngenieroNivel_3(sensores); break;
  case 4: accion = ComportamientoIngenieroNivel_4(sensores); break;
  case 5: accion = ComportamientoIngenieroNivel_5(sensores); break;
  case 6: accion = ComportamientoIngenieroNivel_6(sensores); break;
  }

  return accion;
}


// --------------------------------------------------------------------------------
// Niveles iniciales (Comportamientos reactivos simples)
// --------------------------------------------------------------------------------


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 0

/**
 * @brief Calcula la posición absoluta de la casilla apuntada por un sensor frontal.
 * @param idx índice del sensor (1=45ºizq, 2=frente, 3=45ºdch)
 * @param fil fila actual del agente
 * @param col columna actual del agente
 * @param brujula Orientación del agente (0=N,1=NE,2=E,3=SE,4=S,5=SW,6=W,7=NW)
 */
pair<int,int> PosAbsolutaSensorI(int idx, int fil, int col, int brujula)
{
  /** 
   * Desplazamiento (fila, col) para cada orientación
   * 
   * Dirección 0: N (Norte) → sube una fila, misma columna → df[0] = -1, dc[0] = 0
   * Dirección 1: NE (Noreste) → sube una fila, avanza una columna → df[1] = -1, dc[1] = 1
   * Dirección 2: E (Este) → misma fila, avanza una columna → df[2] = 0, dc[2] = 1
   * Dirección 3: SE (Sureste) → baja una fila, avanza una columna → df[3] = 1, dc[3] = 1
   * Dirección 4: S (Sur) → baja una fila, misma columna → df[4] = 1, dc[4] = 0
   * Dirección 5: SW (Suroeste) → baja una fila, retrocede una columna → df[5] = 1, dc[5] = -1
   * Dirección 6: W (Oeste) → misma fila, retrocede una columna → df[6] = 0, dc[6] = -1
   * Dirección 7: NW (Noroeste) → sube una fila, retrocede una columna → df[7] = -1, dc[7] = -1
  */
  int df[] = {-1, -1, 0, 1, 1, 1, 0, -1};
  int dc[] = {0, 1, 1, 1, 0, -1, -1, -1};

  int dir;
  if (idx == 1) dir = (brujula - 1 + 8) % 8; // 45º izquierda
  else if (idx == 2) dir = brujula; // frente
  else dir = (brujula + 1) % 8; // 45º derecha

  return {fil + df[dir], col + dc[dir]};
}


/**
 * @brief Determina si la casilla es viable por altura
 * @param casilla tipo de terreno
 * @param dif diferencia de altura entre casillas
 * @param zap indica si el agente tiene las zapatillas
 * @return 'P' si no es accesible por altura y casilla en otro caso
 */
char ViablePorAlturaI(char casilla, int dif, bool zap) {
  if (abs(dif)<=1 or (zap and abs(dif)<=2)) return casilla;
  else return 'P';  
}


/**
 * @brief Determina la mejor opción entre las tres casillas que tiene delante, priorizando las no visitadas
 * @param i terreno que tiene en la posición 1 de superficie (45 izq)
 * @param c terreno que tiene en la posición 2 de superficie (justo delante)
 * @param d terreno que tiene en la posición 3 de superficie (45 dch)
 * @param cnt_i/c/d indica cuántas veces ha sido visitada la casilla izq/centro/dch
 * @param zap indica si el agente tiene las zapatillas
 * @return 2 si es mejor WALK, 1 para TURN_SL, 3 para TURN_SR y 0 si no hay nada interesante
 */
int VeoCasillaInteresanteI(char i, char c, char d, bool zap, int cnt_i, int cnt_c, int cnt_d){
  // Meta: siempre tiene la máxima prioridad
  if (c == 'U') return 2;
  if (i == 'U') return 1;
  if (d == 'U') return 3;

  // Zapatillas: solo si no las tenemos aún
  if (!zap) {
    int mejor = -1, min_cnt = INT16_MAX;
    if (c == 'D' and cnt_c < min_cnt) {
      min_cnt = cnt_c;
      mejor = 2;
    }
    if (i == 'D' and cnt_i < min_cnt) {
      min_cnt = cnt_i;
      mejor = 1;
    }
    if (d == 'D' and cnt_d < min_cnt) {
      min_cnt = cnt_d;
      mejor = 3;
    }
    if (mejor != -1) return mejor;
  }

  // Caminos: la casilla 'C' o 'D' menos visitada
  // Tratamos 'D' como camino normal para poder huir por ahí si nos hiciera falta
  int mejor = -1, min_cnt = INT16_MAX;
  bool transitable_c = (c == 'C' or c =='D');
  bool transitable_i = (i == 'C' or i =='D');
  bool transitable_d = (d == 'C' or d =='D');

  if (transitable_c and cnt_c < min_cnt) {
    min_cnt = cnt_c;
    mejor = 2;
  }
  if (transitable_i and cnt_i < min_cnt) {
    min_cnt = cnt_i;
    mejor = 1;
  }
  if (transitable_d and cnt_d < min_cnt) {
    min_cnt = cnt_d;
    mejor = 3;
  }
  if (mejor != -1) return mejor;

  // Resto de casillas: 'P', 'M', 'A', 'H', 'S', 'B', etc.
  return 0;
}


Action ComportamientoIngeniero::ComportamientoIngenieroNivel_0(Sensores sensores)
{
  Action accion = IDLE;
  // El comportamiento de seguir un camino hasta encontrar una plata de T. Residuos
  // Poner el valor de los sensores de visión en el mapa
  ActualizarMapa(sensores);
  
  // Actualización de variables de estado
  if (sensores.superficie[0]=='D') tiene_zapatillas = true;

  // Descripción del comportamiento
  if (sensores.superficie[0]=='U') return IDLE; // Llegué a la meta

  // Acutalizamos que estamos visitando esta casilla en el map
  visitadas[{sensores.posF, sensores.posC}]++;

  // 1. Terminar maniobras pendientes
  if (giro45Izq > 0) {
    giro45Izq--;
    accion = TURN_SL;
    last_action = accion;
    return accion; 
  }

  if (giro45Dch > 0) {
    giro45Dch--;
    accion = TURN_SR;
    last_action = accion;
    return accion; 
  }

  // 2. Evitar colisionar con el técnico (de frente)
  if (sensores.agentes[2] == 't') {
    accion = TURN_SL; // Giramos 45º ahora
    giro45Izq = 1;    // Memorizamos que nos faltan otros 45º para el siguiente ciclo
    last_action = accion;
    return accion;
  }

  // 3. Calcular posiciones absolutas de las tres casillas que detectamos justo delante
  auto abs_i = PosAbsolutaSensorI(1, sensores.posF, sensores.posC, sensores.rumbo);
  auto abs_c = PosAbsolutaSensorI(2, sensores.posF, sensores.posC, sensores.rumbo);
  auto abs_d = PosAbsolutaSensorI(3, sensores.posF, sensores.posC, sensores.rumbo);

  // Almacenamos las casillas visitadas. Si estamos girando buscando salida, penalizamos fuertemente las
  // casillas ya visitadas para que el agente no vuelva por el camino por el que vino hasta completar 360º
  int umbral = (giros_buscando > 0 and giros_buscando < 8) ? 999999 : 0;

  int cnt_i = (visitadas.count(abs_i) and visitadas[abs_i] > 0) ? visitadas[abs_i] + umbral : 0;
  int cnt_c = (visitadas.count(abs_c) and visitadas[abs_c] > 0) ? visitadas[abs_c] + umbral : 0;
  int cnt_d = (visitadas.count(abs_d) and visitadas[abs_d] > 0) ? visitadas[abs_d] + umbral : 0;

  // 4. Ignorar casillas con agente técnico
  // Si el técnico no está de frente, sino en una diagonal (1 o 3) y en esa diagonal también hay una casilla
  // interesante ('U' o 'D'), la función VeoCasillaInteresanteI manda al agente allí y habrá colisión
  // Para arreglarlo mandamos un 'P' directamente en el param casilla en caso de que haya agente
  char cas_i = (sensores.agentes[1] == 't') ? 'P' : sensores.superficie[1];
  char cas_c = (sensores.agentes[2] == 't') ? 'P' : sensores.superficie[2]; // Ya cubierto arriba, pero por coherencia
  char cas_d = (sensores.agentes[3] == 't') ? 'P' : sensores.superficie[3];
  
  // 5. Filtro por altura
  int cota = sensores.cota[0];
  char i = ViablePorAlturaI(cas_i, sensores.cota[1]-cota, tiene_zapatillas);
  char c = ViablePorAlturaI(cas_c, sensores.cota[2]-cota, tiene_zapatillas);
  char d = ViablePorAlturaI(cas_d, sensores.cota[3]-cota, tiene_zapatillas);
/*
  char i_ef = (giros_buscando > 0 && giros_buscando < 8 && cnt_i > 0) ? 'P' : i;
  char c_ef = (giros_buscando > 0 && giros_buscando < 8 && cnt_c > 0) ? 'P' : c;
  char d_ef = (giros_buscando > 0 && giros_buscando < 8 && cnt_d > 0) ? 'P' : d;
*/

  // Toma de decisión
  //int pos = VeoCasillaInteresanteI(i_ef, c_ef, d_ef, tiene_zapatillas, cnt_i, cnt_c, cnt_d);
  int pos = VeoCasillaInteresanteI(i, c, d, tiene_zapatillas, cnt_i, cnt_c, cnt_d);

  switch(pos) {
    case 2:
      accion = WALK;
      giros_buscando = 0;
      break;
    case 1:
      accion = TURN_SL;
      giros_buscando = 0;
      break;
    case 3:
      accion = TURN_SR;
      giros_buscando = 0;
      break;
    default:
      giros_buscando++;
      accion = TURN_SL;
      break;
  }

  last_action = accion;
  return accion;
}


/**
 * @brief Comprueba si una celda es de tipo camino transitable.
 * @param c Carácter que representa el tipo de superficie.
 * @return true si es camino ('C'), zapatillas ('D') o meta ('U').
 */
bool ComportamientoIngeniero::es_camino(unsigned char c) const
{
  return (c == 'C' || c == 'D' || c == 'U');
}
// --------------------------------------------------------------------------------
// ================================================================================


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 1

/**
 * @brief Asigna una cómo de atractivo es un terreno dado
 * @param terreno tipo de terreno
 * @return Puntuación valorando el atractivo de dicho terreno
 */
int ValoraTerrenoI(char terreno) {
  switch (terreno)
  {
    case 'C': case 'S':
      return 5; // Objetivo principal: Caminos y Senderos
      break;
    
    case 'X': case 'U': case 'D':
      return 4; // Otros elementos o casillas útiles
      break;
    
    case 'H':
      return 3; // Hierba: cuesta más energía
      break;
      
    case 'A':
      return 1; // Bosque y Agua: muy costosos
      break;

    case 'P': case 'B': case 'M': default:
      return 0; // Precipicios, Bosques y Muros (Intransitables)
      break;
  }
}


/**
  * @brief Calcula la puntuación efectiva de una casilla combinando valor del terreno y visitas.
  * @param valorTerreno Valor base obtenido de ValoraTerrenoI().
  * @param visitas Número de veces que se ha visitado esa casilla.
  * @return Puntuación float (mayor = más atractiva). Retorna -1.0f si valorTerreno == 0.
  */
float ScoreCasillaI(int valorTerreno, int visitas) {
  if (valorTerreno == 0) return -1.0f;
  // Dividimos el valor base entre (1 + visitas) para penalizar casillas muy transitadas.
  return static_cast<float>(valorTerreno) / (1.0f + visitas);
}


/**
 * @brief Determina la mejor opción para explorar evitando bucles
 * Combina el valor del terreno con una penalización por visitas previas.
 * Más visitas = menos atractivo, terreno mejor = más atractivo.
 * @param i terreno que tiene en la posición 1 de superficie (45 izq)
 * @param c terreno que tiene en la posición 2 de superficie (justo delante)
 * @param d terreno que tiene en la posición 3 de superficie (45 dch)
 * @param cnt_i/c/d indica cuántas veces ha sido visitada la casilla izq/centro/dch
 * @return 2 (WALK), 1 (TURN_SL), 3 (TURN_SR) y 0 si está bloqueado
 */
int VeoCasillaExploracionI(char i, char c, char d, int cnt_i, int cnt_c, int cnt_d) {
  int vi = ValoraTerrenoI(i);
  int vc = ValoraTerrenoI(c);
  int vd = ValoraTerrenoI(d);

  // Si estamos rodeados de obstáculos forzamos un giro
  if (vi==0 and vc==0 and vd==0) return 0;

  // Calculamos puntuaciones
  float si = ScoreCasillaI(vi, cnt_i);
  float sc = ScoreCasillaI(vc, cnt_c);
  float sd = ScoreCasillaI(vd, cnt_d);

  // El centro tiene siempre preferencia (para evitar giros innecesarios)
  if (sc >= si and sc >= sd) return 2;
  if (si >= sd) return 1;
  return 3;
}


/**
 * @brief Comportamiento reactivo del ingeniero para el Nivel 1.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_1(Sensores sensores)
{
  Action accion = IDLE;

  // Actualizamos el mapa con lo que vemos ahora mismo
  ActualizarMapa(sensores);

  // Actualizar variables de estado
  if (sensores.superficie[0]=='D') tiene_zapatillas = true;

  // Acutalizamos que estamos visitando esta casilla en el map
  visitadas[{sensores.posF, sensores.posC}]++;

  // 1. Terminar maniobras pendientes
  if (giro45Izq>0) {
    giro45Izq--;
    accion = TURN_SL;
    last_action = accion;
    return accion;
  }

  if (giro45Dch>0) {
    giro45Dch--;
    accion = TURN_SR;
    last_action = accion;
    return accion;
  }

  // 2. Esquivar técnico de frente
  if (sensores.agentes[2]=='t') {
    accion = TURN_SL;
    giro45Izq = 1; // Recordamos que estamos en mitad del giro
    last_action = accion;
    return accion;
  }

  // 3. Calcular posiciones absolutas de las tres casillas que detectamos justo delante
  auto abs_i = PosAbsolutaSensorI(1, sensores.posF, sensores.posC, sensores.rumbo);
  auto abs_c = PosAbsolutaSensorI(2, sensores.posF, sensores.posC, sensores.rumbo);
  auto abs_d = PosAbsolutaSensorI(3, sensores.posF, sensores.posC, sensores.rumbo);

  // Almacenamos las casillas visitadas
  /*
  int cnt_i = visitadas.count(abs_i) ? visitadas[abs_i] : 0;
  int cnt_c = visitadas.count(abs_c) ? visitadas[abs_c] : 0;
  int cnt_d = visitadas.count(abs_d) ? visitadas[abs_d] : 0;*/
  
  // Almacenamos las casillas visitadas. Si estamos girando buscando salida, penalizamos fuertemente las
  // casillas ya visitadas para que el agente no vuelva por el camino por el que vino hasta completar 360º
  int umbral = (giros_buscando > 0 and giros_buscando < 8) ? 999999 : 0;
  
  int cnt_i = (visitadas.count(abs_i) and visitadas[abs_i] > 0) ? visitadas[abs_i] + umbral : 0;
  int cnt_c = (visitadas.count(abs_c) and visitadas[abs_c] > 0) ? visitadas[abs_c] + umbral : 0;
  int cnt_d = (visitadas.count(abs_d) and visitadas[abs_d] > 0) ? visitadas[abs_d] + umbral : 0;


  // 4. Comprobamos que el técnico no esté en las diagonales
  char cas_i = (sensores.agentes[1] == 't') ? 'P' : sensores.superficie[1];
  char cas_c = (sensores.agentes[2] == 't') ? 'P' : sensores.superficie[2]; // Ya cubierto arriba, pero por coherencia
  char cas_d = (sensores.agentes[3] == 't') ? 'P' : sensores.superficie[3];

  // 5. Filtro por altura
  int cota = sensores.cota[0];
  char i = ViablePorAlturaI(cas_i, sensores.cota[1]-cota, tiene_zapatillas);
  char c = ViablePorAlturaI(cas_c, sensores.cota[2]-cota, tiene_zapatillas);
  char d = ViablePorAlturaI(cas_d, sensores.cota[3]-cota, tiene_zapatillas);

  int pos = VeoCasillaExploracionI(i, c, d, cnt_i, cnt_c, cnt_d);
  
  switch(pos) {
    case 2:
      accion = WALK;
      giros_buscando = 0;
      break;
    case 1:
      accion = TURN_SL;
      giros_buscando = 0;
      break;
    case 3:
      accion = TURN_SR;
      giros_buscando = 0;
      break;
    default:
      giros_buscando++;
      accion = TURN_SL;
      break;
  }

  last_action = accion;
  return accion;
}
// --------------------------------------------------------------------------------
// ================================================================================


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 2

/**
 * @brief Calcula el estado resultante al avanzar una casilla en la dirección actual
 * @param st Estado actual del agente (ubicación y orientación).
 * @return Nuevo estado con la posición avanzada una casilla en la misma orientación.
 */
EstadoI NextCasillaIngeniero(const EstadoI &st) {
  EstadoI next = st;

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
 * @param accion Acción a realizar (WALK o JUMP), pues en esta caso las condiciones cambian
 * @param terreno Matriz de tipos de terreno.
 * @param altura Matriz de cotas (alturas).
 * @return true si la casilla de delante y false en caso contrario.
 */
bool CasillaAccesibleIngeniero(const EstadoI &st, Action accion, const vector<vector<unsigned char>> &terreno,
                             const vector<vector<unsigned char>> &altura) {
  EstadoI next = NextCasillaIngeniero(st);
  int max_diff = st.zapatillas ? 2 : 1;

  // Si la acción es WALK
  if (accion==WALK) {
    char terr = terreno[next.site.f][next.site.c];
    if (terr=='M' or terr=='P' or terr=='B') return false; // intransitables

    int diff = abs(altura[st.site.f][st.site.c]-altura[next.site.f][next.site.c]);

    return diff <= max_diff;
  }

  // Si la acción es JUMP
  if (accion==JUMP) {
    // Comprobamos casilla intermedia
    char terr_mid = terreno[next.site.f][next.site.c]; // El terreno del medio
    if (terr_mid=='M' or terr_mid=='P' or terr_mid=='B') return false; // intransitables
    // cómo meto aquí para comprobar que no haya un agente en la casilla del medio si no tenco acceso a sensores?

    // Comprobamos casilla final
    EstadoI final_jump = NextCasillaIngeniero(next);
    char terr_fin = terreno[final_jump.site.f][final_jump.site.c];
    if (terr_fin=='M' or terr_fin=='P' or terr_fin=='B') return false; // intransitables
    
    // Comprobamos la altura entre la casilla final y la inicial (la del medio da igual (????))
    int diff = abs(altura[st.site.f][st.site.c]-altura[final_jump.site.f][final_jump.site.c]);
    return diff <= max_diff;
  }

  return false;
}

/**
 * @brief Aplica una acción sobre un estado.
 * @param accion Acción a ejecutar (WALK, JUMP, TURN_SR, TURN_SL).
 * @param st Estado actual del agente.
 * @param terreno Matriz de tipos de terreno.
 * @param altura Matriz de cotas.
 * @return EstadoI Nuevo estado tras aplicar la acción.
 */
EstadoI applyI(Action accion, const EstadoI &st, const vector<vector<unsigned char>> &terreno, 
               const vector<vector<unsigned char>> &altura){
  EstadoI next = st;
  switch (accion) {
    case WALK:
      if (CasillaAccesibleIngeniero(st, WALK, terreno, altura))
        next = NextCasillaIngeniero(st);
      break;
    case JUMP:
      if (CasillaAccesibleIngeniero(st, JUMP, terreno, altura))
        next = NextCasillaIngeniero(NextCasillaIngeniero(st));
      break;
    case TURN_SR:
      next.site.brujula = (Orientacion) ((next.site.brujula+1)%8);
      break;
    case TURN_SL:
      next.site.brujula = (Orientacion) ((next.site.brujula+7)%8);
      break;
  }
  
  // Actualizar si recoge zapatillas
  if (terreno[next.site.f][next.site.c] == 'D')
      next.zapatillas = true;

  return next;
}

/**
 * @brief Algoritmo de búsqueda en anchura
 * 
 * @param inicio Estado inicial de la búsqueda
 * @param final Estado final de la búsqueda
 * @param terreno Matriz que contiene la información del terreno
 * @param altura Matriz que contiene las alturas del mapa
 * 
 * @return La secuencia de acciones para llegar al estado final
 * @note Devuelve un plan vacío si no es posible encontrar un plan válido
 */
list<Action> ComportamientoIngeniero::B_Anchura_Nivel2(const EstadoI &inicio, const EstadoI &final, 
                                              const vector<vector<unsigned char>> &terreno,
                                              const vector<vector<unsigned char>> &altura) {
  NodoI current_node;
  list<NodoI> frontier;
  set<NodoI> explored;
  list<Action> path;

  current_node.estado = inicio;
  frontier.push_back(current_node);
  bool SolutionFound = (current_node.estado.site.f == final.site.f and current_node.estado.site.c == final.site.c);

  while(!SolutionFound and !frontier.empty()) {
    frontier.pop_front();
    explored.insert(current_node);

    // Compruebo si estoy en una casilla que da las zapatillas YA HECHO EN applyI
    // if (terreno[current_node.estado.site.f][current_node.estado.site.c]=='D') current_node.estado.zapatillas = true;

    // Genero el hijo resultante de aplicar la acción WALK
    NodoI child_Walk = current_node;
    child_Walk.estado = applyI(WALK, current_node.estado, terreno, altura);
    // Solamente si el estado ha cambiado (es decir, si el movimiento fue válido)
    if (!(child_Walk.estado==current_node.estado)) {
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
    }

    // Genero el hijo resultante de aplicar la acción JUMP
    NodoI child_Jump = current_node;
    child_Jump.estado = applyI(JUMP, current_node.estado, terreno, altura);
    // Solamente si el estado ha cambiado (es decir, si el movimiento fue válido)
    if (!(child_Jump.estado==current_node.estado)) {
      if (child_Jump.estado.site.f==final.site.f and child_Jump.estado.site.c==final.site.c) {
        // El hijo generado es solución
        child_Jump.secuencia.push_back(JUMP);
        current_node = child_Jump;
        SolutionFound = true;
      } else if (explored.find(child_Jump)==explored.end()) {
        // Se mete en la lista de frontier después de añadir la acción JUMP a la secuencia
        child_Jump.secuencia.push_back(JUMP);
        frontier.push_back(child_Jump);
      }
    }
    
    if (!SolutionFound) {
      // El hijo resultante de aplicar la acción TURN_SR
      NodoI child_TurnSR = current_node;
      child_TurnSR.estado = applyI(TURN_SR, current_node.estado, terreno, altura);
      if (explored.find(child_TurnSR)==explored.end()) {
        // Se mete en la lista de frontier después de añadir la acción TURN_SR a la secuencia
        child_TurnSR.secuencia.push_back(TURN_SR);
        frontier.push_back(child_TurnSR);
      }

      // El hijo resultante de aplicar la acción TURN_SL
      NodoI child_TurnSL = current_node;
      child_TurnSL.estado = applyI(TURN_SL, current_node.estado, terreno, altura);
      if (explored.find(child_TurnSL)==explored.end()) {
        // Se mete en la lista de frontier después de añadir la acción TURN_SL a la secuencia
        child_TurnSL.secuencia.push_back(TURN_SL);
        frontier.push_back(child_TurnSL);
      }
    }

    // Evaluamos el siguiente nodo en la frontera, saltando los ya explorados
    if (!SolutionFound and !frontier.empty()) {
      current_node = frontier.front();
      while (explored.find(current_node)!=explored.end() and !frontier.empty()) {
        frontier.pop_front();
        if (!frontier.empty()) current_node = frontier.front();
      }
    }
  }

  if (SolutionFound) path = current_node.secuencia;
  return path;
}


/**
 * @brief Comportamiento del ingeniero para el Nivel 2 (búsqueda).
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_2(Sensores sensores)
{
  Action accion = IDLE;

  if (!hayPlan) {
    // Invocamos al método de búsqueda
    EstadoI inicio, fin;
    inicio.site.f = sensores.posF;
    inicio.site.c = sensores.posC;
    inicio.site.brujula = sensores.rumbo;
    inicio.zapatillas = tiene_zapatillas;
    fin.site.f = sensores.BelPosF;
    fin.site.c = sensores.BelPosC;

    plan = B_Anchura_Nivel2(inicio, fin, mapaResultado, mapaCotas);
    VisualizaPlan(inicio.site, plan);

    hayPlan = (plan.size()!=0);
  }

  if (hayPlan and plan.size()>0) {
    Action next_accion = plan.front();
    
    if (next_accion == WALK and sensores.agentes[2]!='_')
        return IDLE; // El técnico está justo delante, esperamos a que se quite
    
    if (next_accion == JUMP and (sensores.agentes[2]!='_' or sensores.agentes[6]!='_'))
        return IDLE; // El técnico está en medio o en el destino, esperamos a que se quite

    // Si todo está despejado, ejecutamos la acción y la quitamos del plan
    accion = next_accion;
    plan.pop_front();
  }

  if (plan.size()==0) hayPlan = false;

  return accion;
}
// --------------------------------------------------------------------------------
// ================================================================================


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 3
/**
 * @brief Comportamiento del ingeniero para el Nivel 3.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_3(Sensores sensores)
{
  Action accion = IDLE;

  // Buscamos si el técnico está en el campo de visión del ingeniero
  int pos_tecnico = -1;
  for (int i=1; i<sensores.agentes.size(); i++) {
    if (sensores.agentes[i]=='t') {
      pos_tecnico = i;
      break;
    }
  }

  if (pos_tecnico != -1) { // Si lo detecta, apartamos al ingeniero
    // Comprobamos si podemos avanzar de frente
    char sup_frente = sensores.superficie[2];
    bool check1 = (sup_frente != 'M' and sup_frente != 'P' and sensores.agentes[2] == '_');

    ubicacion ub = {sensores.posF, sensores.posC, sensores.rumbo};
    bool check2 = EsAccesiblePorAltura(ub ,tiene_zapatillas);

    bool puedo_avanzar = check1 and check2;

    if (puedo_avanzar and pos_tecnico != 2 and pos_tecnico != 6 and pos_tecnico != 12) 
      accion = WALK; // Cuando ir de frente es seguro y el técnico NO está justo en nuestro camino
    else { // Cuando no podemos ir de frente
      // Evaluar opciones laterales
      // Identificamos si el técnico está en la mitad izquierda de nuestra visión:
      bool tecnico_izq = (pos_tecnico == 1 or pos_tecnico == 4 or pos_tecnico == 5 or 
                          (pos_tecnico >= 9 and pos_tecnico <= 11));
      if (tecnico_izq) accion = TURN_SR;  // Técnico a la izquierda, entonces giro a derecha
      else  accion = TURN_SL; // Técnico a la derecha, entonces giro a izquierda
    }
  }

  return accion;
}
// --------------------------------------------------------------------------------
// ================================================================================


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 4

/**
 * @brief Extrae los costes de energía y ecología según el terreno y la operación
 */
void ObtenerCostesTubo(char terreno, int op, int &energia, int &eco) {
  int install_energ = 30, install_eco = 30;
  int raise_energ = 40, raise_eco = 40;
  int dig_energ = 50, dig_eco = 50;

  switch(terreno) {
    case 'A':
      install_energ = 60, install_eco = 50;
      raise_energ = 0, raise_eco = 0;
      dig_energ = 0, dig_eco = 0;
      break;
    case 'H':
      install_energ = 45, install_eco = 45;
      raise_energ = 55, raise_eco = 55;
      dig_energ = 65, dig_eco = 65;
      break;
    case 'S':
      install_energ = 25, install_eco = 25;
      raise_energ = 30, raise_eco = 30;
      dig_energ = 40, dig_eco = 40;
      break;      
    case 'C': case 'U':
      install_energ = 15, install_eco = 15;
      raise_energ = 10, raise_eco = 10;
      dig_energ = 25, dig_eco = 25;
      break;
  }

  energia = install_energ*2;
  eco = install_eco*2;

  if (op == 1) {  // RAISE
    energia += raise_energ;
    eco += raise_eco;
  } else if (op == -1) {  // DIG
    energia += dig_energ;
    eco += dig_eco;
  }
}

/**
 * @brief Comprueba si la operación de altura es válida en el terreno indicado
 */
bool EsOpcionValida(char terreno, int altura, int op) {
  if (terreno == 'M' or terreno == 'P' or terreno == 'B') return false; // no se puede construir en muros, precipicios o bosques
  if (op == 1 and (terreno == 'A' or altura >= 9)) return false;  // RAISE
  if (op == -1 and (terreno == 'A' or altura <= 1)) return false; // DIG
  return true;
}


/**
 * @brief Calcula la distancia Manhattan desde una posición dada hasta el conjunto casillas 'U'.
 * @param f Fila de la posición actual del agente.
 * @param c Columna de la posición actual del agente.
 * @param plantas_U Vector de pares (fila, columna) que representan las ubicaciones de las plantas 'U'.
 * @return Distancia Manhattan mínima hasta una casilla 'U' en el vector.
 */
int HeuristicaTubo(int f, int c, const vector<pair<int, int>> &plantas_U) {
  int min_dist = 999999;
  for (const auto& planta : plantas_U) {
    int dist = abs(f-planta.first) + abs(c-planta.second);
    if (dist < min_dist) min_dist = dist;
  }

  return min_dist;
}


list<Paso> ComportamientoIngeniero::PlanificarTuberias_AStar(int f_bel, int c_bel, int max_energia, int max_eco, 
                                                             const vector<vector<unsigned char>> &terreno, 
                                                             const vector<vector<unsigned char>> &altura) 
{
  list<Paso> plan;
  priority_queue<NodoTubo> frontier;
  map<EstadoTubo, vector<InfoTrayecto>> explored;  // guarda una lista de trayectos por estado

  // Precalculamos las posiciones de 'U' para la heuristica
  vector<pair<int, int>> plantas_U;
  for (int i=0; i<terreno.size(); i++) {
    for (int j=0; j<terreno[i].size(); j++) {
      if (terreno[i][j] == 'U') plantas_U.push_back({i,j});
    }
  }

  // Inicializamos insertando hasta tres nodos iniciales 
  for (int op=-1; op<=1; op++) {
    char terr = terreno[f_bel][c_bel];
    int alt = altura[f_bel][c_bel] - '0';

    if (EsOpcionValida(terr, alt, op)) {
      NodoTubo start;
      start.estado = {f_bel, c_bel, op};
      ObtenerCostesTubo(terr, op, start.energia_gastada, start.eco_acumulado);

      // Si el origen ya excede los límites lo descartamos
      if (start.energia_gastada > max_energia or start.eco_acumulado > max_eco) continue;

      start.coste_g = 1; // 1 tramo
      start.coste_h = HeuristicaTubo(f_bel, c_bel, plantas_U);
      start.coste_f = start.coste_g + start.coste_h;
      start.secuencia.push_back({f_bel, c_bel, op});

      frontier.push(start);
    }
  }

  int df[] = {-1, 1, 0, 0};
  int dc[] = {0, 0, -1, 1};

  while(!frontier.empty()) {
    NodoTubo current = frontier.top();
    frontier.pop();

    // Condición para acabar: cuando llegamos a la planta 'U'
    if (terreno[current.estado.f][current.estado.c] == 'U') return current.secuencia;

    // Control de explorados
    bool dominado = false;
    for (const auto &hist : explored[current.estado]) {
      // Si ya hemos visitado este estado con menor o igual coste, energía y ecología, este camino no vale la pena
      if (hist.g <= current.coste_g and hist.energia <= current.energia_gastada and hist.eco <= current.eco_acumulado) {
        dominado = true;
        break;
      }
    }
    if (dominado) continue; // lo podamos

    // Si sobrevive, lo registramos en el historial de este estado
    explored[current.estado].push_back({current.coste_g, current.energia_gastada, current.eco_acumulado});

    
    // Generamos hijos (next)
    for (int i=0; i<4; i++) {
      int n_f = current.estado.f + df[i];
      int n_c = current.estado.c + dc[i];

      // Comprobamos que no nos hemos salido del mapa
      if (n_f < 0 or n_f >= terreno.size() or n_c < 0 or n_c >= terreno[0].size()) continue;

      char n_terr = terreno[n_f][n_c];
      int n_alt = altura[n_f][n_c] - '0';

      // Probamos las tres modificaciones posibles en la casilla destino
      for (int n_op=-1; n_op<=1; n_op++) {
        if (EsOpcionValida(n_terr, n_alt, n_op)) {
          int alt_mod_current = (altura[current.estado.f][current.estado.c] - '0') + current.estado.op_aplicada;
          int alt_mod_next = n_alt + n_op;

          // El origen debe ser igual o una unidad mayor que el destino
          int diff_altura = alt_mod_current - alt_mod_next;
          if (diff_altura == 0 or diff_altura == 1) {
            int coste_energ, coste_eco;
            ObtenerCostesTubo(n_terr, n_op, coste_energ, coste_eco);

            NodoTubo child = current;
            child.estado = {n_f, n_c, n_op};
            child.energia_gastada += coste_energ;
            child.eco_acumulado += coste_eco;

            // Poda por límites de consumo
            if (child.energia_gastada > max_energia or child.eco_acumulado > max_eco) continue;

            child.coste_g += 1;
            child.coste_h = HeuristicaTubo(n_f, n_c, plantas_U);
            child.coste_f = child.coste_g + child.coste_h;
            child.secuencia.push_back({n_f, n_c, n_op});

            frontier.push(child);
          }
        }
      }
    }
  }

  return plan;
}


/**
 * @brief Comportamiento del ingeniero para el Nivel 4.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_4(Sensores sensores)
{
  // Si ya hemos calculado y validado el plan, no hay que hacer nada más
  if (hayPlan) return IDLE;

  // Extraemos los datos del mundo
  int f_bel = sensores.BelPosF;
  int c_bel = sensores.BelPosC;
  int max_energia = sensores.energia;
  int max_eco = sensores.max_ecologico;

  // Ejecutar el planificador de Tuberías
  list<Paso> plan_tuberias = PlanificarTuberias_AStar(f_bel, c_bel, max_energia, max_eco, mapaResultado, mapaCotas);
    
  if (!plan_tuberias.empty()) {
    VisualizaRedTuberias(plan_tuberias);
    hayPlan = true;
  }

  return IDLE;
}
// --------------------------------------------------------------------------------
// ================================================================================


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 5

/**
 * @brief Comprueba si desde (f1,c1) con orientación rumbo se apunta a (f2,c2)
 * @note Asume adyacencia ortogonal ya verificada
 */
Orientacion RumboHaciaI(int f1, int c1, int f2, int c2) {
  if (f1 > f2) return norte;
  if (f1 < f2) return sur;
  if (c1 > c2) return oeste;
  if (c1 < c2) return este;
  return norte;
}

/**
 * @brief Comportamiento del ingeniero para el Nivel 5.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_5(Sensores sensores)
{
  switch (estado_eng)
  {
  case ENG_PLANIFICAR_RED: {
    // Extraemos los datos del mundo
    int max_energia = sensores.energia;
    int max_eco = sensores.max_ecologico;
    int f_bel = sensores.BelPosF;
    int c_bel = sensores.BelPosC;

    // Ejecutamos el plan (nivel 4)
    listaCanalizacionTuberias = PlanificarTuberias_AStar(f_bel, c_bel, max_energia, max_eco, mapaResultado, mapaCotas);

    // Si el plan ha sido exitoso (no es vacío), pasamos a fase de ejecución
    if (!listaCanalizacionTuberias.empty()) estado_eng = ENG_INIT_TRAMO;

    return IDLE; // gastamos este turno en pensar
  }

  case ENG_INIT_TRAMO: {
    // Si nos quedamos con menos de dos casillas, hemos terminado el plan
    if (listaCanalizacionTuberias.size() < 2) return IDLE;

    // Extraemos las dos siguientes casillas
    auto it = listaCanalizacionTuberias.begin();
    Paso c1 = *it;
    Paso c2 = *next(it);
    casilla_tech_preparada = false; // primer tramo: nadie la ha operado

    /*
    // Desempate de alturas (no sé si es necesario)
    int alt1 = (mapaCotas[c1.fil][c1.col] - '0') + c1.op;
    int alt2 = (mapaCotas[c2.fil][c2.col] - '0') + c2.op;

    if (alt1 <= alt2+1) {
      casilla_eng = c1;
      casilla_tech = c2;
    } else {
      casilla_eng = c2;
      casilla_tech = c1;
    }*/
    casilla_eng = c2;
    casilla_tech = c1;

    estado_eng = ENG_IR_A_TECH;
    plan.clear();
    return IDLE;
  }

  case ENG_IR_A_TECH: {
    if (sensores.choque) {
      // Calculamos dónde está el obstáculo
      int f_obst = sensores.posF, c_obst = sensores.posC;
      switch (sensores.rumbo)
      {
        case norte: f_obst--; break;
        case noreste: f_obst--; c_obst++; break;
        case este: c_obst++; break;
        case sureste: f_obst++; c_obst++; break;
        case sur: f_obst++; break;
        case suroeste: f_obst++; c_obst--; break;
        case oeste: c_obst--; break;
        case noroeste: f_obst--; c_obst--; break;
      }

      // Si el obstáculo es nuestro destino final esperamos
      if (f_obst == casilla_tech.fil and c_obst == casilla_tech.col) {
        plan.clear();
        estado_eng = ENG_PREPARAR_TECH;
        return IDLE;
      }

      if (sensores.posF == casilla_tech.fil and sensores.posC == casilla_tech.col) {
        plan.clear();
        estado_eng = ENG_PREPARAR_TECH;
        return IDLE;
      }

      // Marcamos la casilla con 'M' temporalmente
      unsigned char orig1 = mapaResultado[f_obst][c_obst];
      mapaResultado[f_obst][c_obst] = 'M';

      EstadoI inicio = {sensores.posF, sensores.posC, sensores.rumbo};
      EstadoI final = {casilla_tech.fil, casilla_tech.col, norte};
      plan = B_Anchura_Nivel2(inicio, final, mapaResultado, mapaCotas);

      // Restauramos el mapa
      mapaResultado[f_obst][c_obst] = orig1;

      if (!plan.empty()) {
        Action a = plan.front();
        plan.pop_front();
        return a;
      }

      return IDLE;
    }

    if (sensores.posF == casilla_tech.fil and sensores.posC == casilla_tech.col) {
      plan.clear();
      estado_eng = ENG_PREPARAR_TECH;
      return IDLE;
    }

    if (plan.empty()) {
      EstadoI inicio = {sensores.posF, sensores.posC, sensores.rumbo};
      EstadoI final = {casilla_tech.fil, casilla_tech.col, norte};
      plan = B_Anchura_Nivel2(inicio, final, mapaResultado, mapaCotas);
    }

    if (!plan.empty()) {
      Action a = plan.front();
      plan.pop_front();
      return a;
    }

    return IDLE;
  }

  case ENG_PREPARAR_TECH: {
    estado_eng = ENG_LLAMAR_TECH;
    if (!casilla_tech_preparada) { // solo si no fue ya operada
      if (casilla_tech.op == 1) return RAISE;
      if (casilla_tech.op == -1) return DIG;
    }
    casilla_tech_preparada = false; // reset para el siguiente tramo
    return IDLE;
  }

  case ENG_LLAMAR_TECH: {
    estado_eng = ENG_IR_A_ENG;
    plan.clear();
    return COME;
  }

  case ENG_IR_A_ENG: {
    if (sensores.choque) {
      // Calculamos dónde está el obstáculo
      int f_obst = sensores.posF, c_obst = sensores.posC;
      switch (sensores.rumbo)
      {
        case norte: f_obst--; break;
        case noreste: f_obst--; c_obst++; break;
        case este: c_obst++; break;
        case sureste: f_obst++; c_obst++; break;
        case sur: f_obst++; break;
        case suroeste: f_obst++; c_obst--; break;
        case oeste: c_obst--; break;
        case noroeste: f_obst--; c_obst--; break;
      }

      // Si el obstáculo es nuestro destino final esperamos
      if (f_obst == casilla_eng.fil and c_obst == casilla_eng.col) {
        plan.clear();
        return IDLE;
      }

      // Marcamos la casilla con 'M' temporalmente
      unsigned char original = mapaResultado[f_obst][c_obst];
      mapaResultado[f_obst][c_obst] = 'M';

      EstadoI inicio = {sensores.posF, sensores.posC, sensores.rumbo};
      EstadoI final = {casilla_eng.fil, casilla_eng.col, norte};
      plan = B_Anchura_Nivel2(inicio, final, mapaResultado, mapaCotas);

      // Restauramos el mapa
      mapaResultado[f_obst][c_obst] = original;

      if (!plan.empty()) {
        Action a = plan.front();
        plan.pop_front();
        return a;
      }
      
      return IDLE;
    }

    if (sensores.posF == casilla_eng.fil and sensores.posC == casilla_eng.col) {
      plan.clear();
      estado_eng = ENG_PREPARAR_ENG;
      return IDLE;
    }

    if (plan.empty()) {
      EstadoI inicio = {sensores.posF, sensores.posC, sensores.rumbo};
      EstadoI final = {casilla_eng.fil, casilla_eng.col, norte};
      plan = B_Anchura_Nivel2(inicio, final, mapaResultado, mapaCotas);
    }

    if (!plan.empty()) {
      Action a = plan.front();
      plan.pop_front();
      return a;
    }

    return IDLE;
  }

  case ENG_PREPARAR_ENG: {
    estado_eng = ENG_ESPERAR_SYNC;
    if (casilla_eng.op == 1) return RAISE;
    if (casilla_eng.op == -1) return DIG;
    return IDLE;
  }
  
  case ENG_ESPERAR_SYNC: {
    // Debemos determinar la posición de la casilla del técnico con respecto a la del ingeniero
    Orientacion orient = RumboHaciaI(sensores.posF, sensores.posC, casilla_tech.fil, casilla_tech.col);
      
    // Giramos buscando al técnico por el "rumbo más corto"
    if (sensores.rumbo != orient) {
      int diff = ((int)orient - (int)sensores.rumbo + 8) % 8;
      return (diff <= 4) ? TURN_SR : TURN_SL;
    }

    if (sensores.enfrente) {
      listaCanalizacionTuberias.pop_front();

      if (listaCanalizacionTuberias.size() < 2) {
        estado_eng = ENG_INIT_TRAMO; // estado final, ya no hay más
      } else {
        auto it = listaCanalizacionTuberias.begin();
        casilla_eng = *next(it);
        casilla_tech = *it;
        casilla_tech_preparada = true; // ya fue RAISE/DIG como casilla_eng
        estado_eng = ENG_IR_A_TECH;
        plan.clear();
      }

      return INSTALL;
    }

    return IDLE;
  }

  } // switch

  return IDLE;
}
// --------------------------------------------------------------------------------
// ================================================================================


// ================================================================================
// --------------------------------------------------------------------------------
// NIVEL 6
/**
 * @brief Comportamiento del ingeniero para el Nivel 6.
 * @param sensores Datos actuales de los sensores.
 * @return Acción a realizar.
 */
Action ComportamientoIngeniero::ComportamientoIngenieroNivel_6(Sensores sensores)
{
  return IDLE;
}
// --------------------------------------------------------------------------------
// ================================================================================



// =========================================================================
// FUNCIONES PROPORCIONADAS
// =========================================================================

/**
 * @brief Actualiza el mapaResultado y mapaCotas con la información de los sensores.
 * @param sensores Datos actuales de los sensores.
 */
void ComportamientoIngeniero::ActualizarMapa(Sensores sensores)
{
  mapaResultado[sensores.posF][sensores.posC] = sensores.superficie[0];
  mapaCotas[sensores.posF][sensores.posC] = sensores.cota[0];

  int pos = 1;
  switch (sensores.rumbo)
  {
  case norte:
    for (int j = 1; j < 4; j++)
      for (int i = -j; i <= j; i++)
      {
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
      for (int i = -j; i <= j; i++)
      {
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
      for (int i = -j; i <= j; i++)
      {
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
      for (int i = -j; i <= j; i++)
      {
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
 * @brief Determina si una casilla es transitable para el ingeniero.
 * @param f Fila de la casilla.
 * @param c Columna de la casilla.
 * @param tieneZapatillas Indica si el agente posee las zapatillas.
 * @return true si la casilla es transitable (no es muro ni precipicio).
 */
bool ComportamientoIngeniero::EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas)
{
  if (f < 0 || f >= mapaResultado.size() || c < 0 || c >= mapaResultado[0].size())
    return false;
  return es_camino(mapaResultado[f][c]); // Solo 'C', 'D', 'U' son transitables en Nivel 0
}

/**
 * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
 * Para el ingeniero: desnivel máximo 1 sin zapatillas, 2 con zapatillas.
 * @param actual Estado actual del agente (fila, columna, orientacion, zap).
 * @return true si el desnivel con la casilla de delante es admisible.
 */
bool ComportamientoIngeniero::EsAccesiblePorAltura(const ubicacion &actual, bool zap)
{
  ubicacion del = Delante(actual);
  if (del.f < 0 || del.f >= mapaCotas.size() || del.c < 0 || del.c >= mapaCotas[0].size())
    return false;
  int desnivel = abs(mapaCotas[del.f][del.c] - mapaCotas[actual.f][actual.c]);
  if (zap && desnivel > 2)
    return false;
  if (!zap && desnivel > 1)
    return false;
  return true;
}

/**
 * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
 * Calcula la casilla frontal según la orientación actual (8 direcciones).
 * @param actual Estado actual del agente (fila, columna, orientacion).
 * @return Estado con la fila y columna de la casilla de enfrente.
 */
ubicacion ComportamientoIngeniero::Delante(const ubicacion &actual) const
{
  ubicacion delante = actual;
  switch (actual.brujula)
  {
  case 0:
    delante.f--;
    break; // norte
  case 1:
    delante.f--;
    delante.c++;
    break; // noreste
  case 2:
    delante.c++;
    break; // este
  case 3:
    delante.f++;
    delante.c++;
    break; // sureste
  case 4:
    delante.f++;
    break; // sur
  case 5:
    delante.f++;
    delante.c--;
    break; // suroeste
  case 6:
    delante.c--;
    break; // oeste
  case 7:
    delante.f--;
    delante.c--;
    break; // noroeste
  }
  return delante;
}

/**
 * @brief Imprime por consola la secuencia de acciones de un plan.
 *
 * @param plan  Lista de acciones del plan.
 */
void ComportamientoIngeniero::PintaPlan(const list<Action> &plan)
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
 * @brief Imprime las coordenadas y operaciones de un plan de tubería.
 *
 * @param plan  Lista de pasos (fila, columna, operación),
 *              donde operacion = -1 (DIG), operación = 1 (RAISE).
 */
void ComportamientoIngeniero::PintaPlan(const list<Paso> &plan)
{
  auto it = plan.begin();
  while (it != plan.end())
  {
    cout << it->fil << ", " << it->col << " (" << it->op << ")\n";
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
void ComportamientoIngeniero::VisualizaPlan(const ubicacion &st,
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

/**
 * @brief Convierte un plan de tubería en la lista de casillas usada
 *        por el sistema de visualización.
 *
 * @param st    Estado de partida (no utilizado directamente).
 * @param plan  Lista de pasos del plan de tubería.
 */
void ComportamientoIngeniero::VisualizaRedTuberias(const list<Paso> &plan)
{
  listaCanalizacionTuberias.clear();
  auto it = plan.begin();
  while (it != plan.end())
  {
    listaCanalizacionTuberias.push_back({it->fil, it->col, it->op});
    it++;
  }
}
