#ifndef HILO_PLANIFICADOR_KERNEL_H_
#define HILO_PLANIFICADOR_KERNEL_H_

#include <commons/config.h>

#include "utils.h"

extern int contador_pid; // Contador. Para asignar diferente pid a cada nuevo proceso.

/**
* @brief Inicia el Planificador de corto plazo.
*/
void iniciar_planificador(void);

////////////////////////////////////////////

/////// ALGORITMOS

// EN DESARROLLO
void planific_corto_fifo(void);

// EN DESARROLLO
void planific_corto_prioridades(void);

// EN DESARROLLO
void planific_corto_multinivel(void);

// DEL TP VIEJO, COMO REFERENCIA
void planific_corto_rr(void);


// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

/**
* @brief Manda a ejecutar en CPU el siguiente Hilo de una cola READY (ya planificada).
*        Su correspondiente TCB queda en estado EXEC.
* @param cola_ready : La cola Ready de la cual se quiera poner a ejecutar el Hilo.
* @note  Asume que en ese momento no hay Hilo en ejecución (el CPU está esperando).
*/
void ejecutar_siguiente_hilo(t_list* cola_ready);


t_list* recibir_de_cpu(int* codigo_operacion);



// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

/**
* @brief Crea un nuevo Proceso, y su Hilo main.
* @param tamanio                : El tamanio del Proceso a crear.
* @param prioridad_hilo_main    : La prioridad del Hilo main del Proceso a crear.
* @param path_instruc_hilo_main : El path al archivo de instrucciones del Hilo
*                                 main del Proceso a crear.
* @return : El PCB del Proceso creado.
* @note El Proceso y el Hilo creados no pertenecen a ningún estado. Se deben
*       activar luego, moviendo el Proceso a NEW.
*/
t_pcb* nuevo_proceso(int tamanio, int prioridad_hilo_main, char* path_instruc_hilo_main);

void ingresar_a_new(t_pcb* pcb);

/**
* @brief Crea un nuevo Hilo, y lo asocia con el Proceso que ordenó crearlo.
* @param pcb_creador        : El PCB del Proceso creador.
* @param prioridad          : La prioridad del Hilo a crear.
* @param path_instrucciones : El path al archivo de instrucciones del Hilo a crear.
* @return : El TCB del Hilo creado.
* @note El Hilo creado no pertenece a ningún estado. Se debe activar luego,
*       moviéndolo a READY.
*/
t_tcb* nuevo_hilo(t_pcb* pcb_creador, int prioridad, char* path_instrucciones);

/**
* @brief Pone un Hilo en READY, replanificando según algoritmo FIFO.
* @param tcb : El TCB del Hilo a poner en READY.
* @note "replanificar" conceptualmente, pues en realidad ya lo ingresa
*       ordenado en la cola.
*/
void ingresar_a_ready_fifo(t_tcb* tcb);
/**
* @brief Pone un Hilo en READY, replanificando según algoritmo PRIORIDADES.
* @param tcb : El TCB del Hilo a poner en READY.
* @note "replanificar" conceptualmente, pues en realidad ya lo ingresa
*       ordenado en la cola.
*/
void ingresar_a_ready_prioridades(t_tcb* tcb);
/**
* @brief Pone un Hilo en READY, replanificando según algoritmo CMN. En caso
*        de no existir la cola de READY correspondiente a la prioridad del Hilo
*        a ingresar, esta es creada.
* @param tcb : El TCB del Hilo a poner en READY.
* @note "replanificar" conceptualmente, pues en realidad lo ingresa
*       ordenado en la cola que corresponda.
*/
void ingresar_a_ready_multinivel(t_tcb* tcb);
/**
* @brief Envia a CPU la orden para ejecutar instrucciones de un Hilo.
* @param tcb : El TCB del Hilo a correr.
*/
void enviar_orden_de_ejecucion_al_cpu(t_tcb* tcb);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

t_pcb* crear_pcb(int pid, int tamanio);

t_cola_ready* crear_ready_multinivel();

/* OBSOLETO. --------------
t_recurso* encontrar_recurso_del_sistema(char* nombre);
t_recurso_ocupado* encontrar_recurso_ocupado(t_list* lista_de_recursos_ocupados, char* nombre);
t_recurso_blocked* encontrar_recurso_blocked(char* nombre);
void asignar_recurso_ocupado(t_pcb* pcb, char* nombre_recurso);
// void* planificador_largo(t_parametros_planif_largo arg); //funcion para pasar a hilo, cuando se necesita al planificador de largo plazo se crea el hilo y se le da un opcode dependiendo del requisito.
---------------------------
*/

#endif
