#ifndef COMPORTAMIENTOINGENIERO_H
#define COMPORTAMIENTOINGENIERO_H

#include <chrono>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <time.h>

#include "comportamientos/comportamiento.hpp"


struct EstadoI {
  ubicacion site;
  bool zapatillas;

  bool operator==(const EstadoI &st) const {
    return site==st.site and zapatillas==st.zapatillas;
  }
};

struct NodoI {
  EstadoI estado;
  list<Action> secuencia;

  bool operator==(const NodoI &node) const {
    return estado==node.estado;
  }

  bool operator<(const NodoI &node) const {
    if (estado.site.f < node.estado.site.f) return true;
    else if (estado.site.f == node.estado.site.f and estado.site.c < node.estado.site.c) return true;
    else if (estado.site.f == node.estado.site.f and estado.site.c == node.estado.site.c and
             estado.site.brujula < node.estado.site.brujula) return true;
    else if (estado.site.f == node.estado.site.f and estado.site.c == node.estado.site.c and
             estado.site.brujula == node.estado.site.brujula and estado.zapatillas < node.estado.zapatillas) return true;
    else return false;
  }
};

// Estructura para guardar el historial de recursos con los que llegamos a un nodo
struct InfoTrayecto {
  int g;
  int energia;
  int eco;
};

struct EstadoTubo {
  int f;
  int c;
  int op_aplicada;

  bool operator==(const EstadoTubo &st) const {
    return f==st.f and c==st.c and op_aplicada==st.op_aplicada;
  }

  bool operator<(const EstadoTubo &st) const {
    if (f < st.f) return true;
    else if (f == st.f and c < st.c) return true;
    else if (f == st.f and c == st.c  and op_aplicada < st.op_aplicada) return true;
    return false;
  }
};

struct NodoTubo {
  EstadoTubo estado;
  list<Paso> secuencia;

  // Costes para el A*
  int coste_g;  // Número de tramos (acciones INSTALL), que es lo que queremos minimizar
  int coste_h;  // Distancia Manhattan a la 'U' más cercana
  int coste_f;  // g + h (coste total estimado)

  // Presupuestos acumulados
  int energia_gastada;
  int eco_acumulado;

  bool operator==(const NodoTubo &node) const {
    return estado == node.estado;
  }

  bool operator<(const NodoTubo &node) const {
    // priority_queue extrae el elemento MAYOR por defecto. Al usar '>' la cola extraerá siempre 
    // el nodo con el MENOR coste de tiempo (es decir que estamos simulando una min-heap)
    if (coste_f > node.coste_f) return true;

    if (coste_f == node.coste_f) {
      // A igual coste total, preferimos mayor g (estamos más cerca de la meta y reducimos la heurística)
      if (coste_g < node.coste_g) return true;

      // En caso de que tengan el mismo coste g, preferimos el que ha gastado menos impacto ecológico
      if (coste_g == node.coste_g and eco_acumulado > node.eco_acumulado) return true;

      // En caso de empate, preferimos el que ha gastado menos energía
      if (coste_g == node.coste_g and eco_acumulado == node.eco_acumulado and energia_gastada > node.energia_gastada) return true;
    }

    return false;
  }
};



class ComportamientoIngeniero : public Comportamiento {
public:
  // =========================================================================
  // CONSTRUCTORES
  // =========================================================================
  
  /**
   * @brief Constructor para niveles 0, 1 y 6 (sin mapa completo)
   * @param size Tamaño del mapa (si es 0, se inicializa más tarde)
   */
  ComportamientoIngeniero(unsigned int size = 0) : Comportamiento(size) {
    // Inicializar Variables de Estado
    last_action = IDLE;
    tiene_zapatillas = false;
    giro45Izq = 0;
    giro45Dch = 0;
  }

  /**
   * @brief Constructor para niveles 2, 3, 4 y 5 (con mapa completo conocido)
   * @param mapaR Mapa de terreno conocido
   * @param mapaC Mapa de cotas conocido
   */
  ComportamientoIngeniero(std::vector<std::vector<unsigned char>> mapaR, 
                         std::vector<std::vector<unsigned char>> mapaC): 
                         Comportamiento(mapaR, mapaC) {
    // Inicializar Variables de Estado
    hayPlan = false;
    tiene_zapatillas = false;
  }

  ComportamientoIngeniero(const ComportamientoIngeniero &comport)
      : Comportamiento(comport) {}
  ~ComportamientoIngeniero() {}

  /**
   * @brief Bucle principal de decisión del agente.
   * Estudia los sensores y decide la siguiente acción.
   * 
   * EJEMPLO DE USO:
   * Action accion = think(sensores);
   * return accion; // El motor ejecutará esta acción
   */
  Action think(Sensores sensores);

  ComportamientoIngeniero *clone() {
    return new ComportamientoIngeniero(*this);
  }

  // =========================================================================
  // ÁREA DE IMPLEMENTACIÓN DEL ESTUDIANTE
  // =========================================================================

  // Funciones específicas para cada nivel (para ser implementadas por el alumno)
  
  /**
   * @brief Implementación del Nivel 0.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_0(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 1.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_1(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 2.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */ 
  Action ComportamientoIngenieroNivel_2(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 3.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_3(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 4.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_4(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 5.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_5(Sensores sensores);
  
  /**
   * @brief Implementación del Nivel 6.
   * @param sensores Datos actuales de los sensores del agente.
   * @return Acción a realizar.
   */
  Action ComportamientoIngenieroNivel_6(Sensores sensores);

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
  list<Action> B_Anchura_Nivel2(const EstadoI &inicio, const EstadoI &final, 
                                const vector<vector<unsigned char>> &terreno,
                                const vector<vector<unsigned char>> &altura
  );

  list<Paso> PlanificarTuberias_AStar(int f_bel, int c_bel, int max_energia, int max_eco, 
                                      const vector<vector<unsigned char>> &terreno, 
                                      const vector<vector<unsigned char>> &altura
  );

protected:
  // =========================================================================
  // FUNCIONES PROPORCIONADAS
  // =========================================================================

  /**
   * @brief Actualiza la información del mapa interno basándose en los sensores.
   * IMPORTANTE: Esta función ya está implementada. Actualiza mapaResultado y mapaCotas
   * con la información de los 16 sensores (casilla actual + 15 casillas alrededor).
   */
  void ActualizarMapa(Sensores sensores);

  /**
   * @brief Comprueba si una casilla es transitable.
   * @param f Fila de la casilla.
   * @param c Columna de la casilla.
   * @param tieneZapatillas Indica si el agente posee zapatillas.
   * @return true si la casilla es transitable (no es muro ni precipicio).
   */
  bool EsCasillaTransitableLevel0(int f, int c, bool tieneZapatillas);

  /**
   * @brief Comprueba si la casilla de delante es accesible por diferencia de altura.
   * REGLAS: Desnivel máximo 1 sin zapatillas, 2 con zapatillas.
   * @param actual Estado actual del agente (fila, columna, orientacion).
   * @return true si el desnivel con la casilla de delante es admisible.
   */
  bool EsAccesiblePorAltura(const ubicacion &actual, bool zap);

  /**
   * @brief Devuelve la posición (fila, columna) de la casilla que hay delante del agente.
   * @param actual Estado actual del agente (fila, columna, orientacion).
   * @return Estado con la fila y columna de la casilla de enfrente.
   */
  ubicacion Delante(const ubicacion &actual) const;

  bool es_camino(unsigned char c) const;

  /**
 * @brief Imprime por consola la secuencia de acciones de un plan para un agente.
 * @param plan  Lista de acciones del plan.
 */
  void PintaPlan(const list<Action> &plan);


/**
 * @brief Imprime las coordenadas y operaciones de un plan de tubería.
 * @param plan  Lista de pasos (fila, columna, operación).
 */
  void PintaPlan(const list<Paso> &plan);


  /**
 * @brief Convierte un plan de acciones en una lista de casillas para
 *        su visualización en el mapa gráfico.
 * @param st    Estado de partida.
 * @param plan  Lista de acciones del plan.
 */
  void VisualizaPlan(const ubicacion &st, const list<Action> &plan);

  /**
 * @brief Convierte un plan de tubería en la lista de casillas usada
 *        por el sistema de visualización.
 * @param st    Estado de partida (no utilizado directamente).
 * @param plan  Lista de pasos del plan de tubería.
 */
  void VisualizaRedTuberias(const list<Paso> &plan);



private:
  // =========================================================================
  // VARIABLES DE ESTADO (PUEDEN SER EXTENDIDAS POR EL ALUMNO)
  // =========================================================================

Action last_action;     // Almacena la última acción realizada
bool tiene_zapatillas;  // Indica si el agente tiene las zapatillas
int giro45Izq;          // Indica el número de giros a la izqiuerda que quedan por dar
int giro45Dch;          // Indica el número de giros a la derecha que quedan por dar
map<pair<int,int>, int> visitadas;
// set<pair<int,int>> visitadas;


bool hayPlan;            // Indica si hay una plan que ejecutar
list<Action> plan;       // Almacena el plan a realizar.
};

#endif
