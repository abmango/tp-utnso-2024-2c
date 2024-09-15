#include <new.h>

// variables globales para este hilo:
// ----------------------------------
char* ip_memoria = NULL;
char* puerto_memoria = NULL;
// ----------------------------------

void* rutina_new(void* puntero_null) {

// DESARROLLANDO...

    t_pcb* pcb = NULL;
    t_pcb* tcb = NULL;
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    bool exito_al_inicializar_proceso = false;
    bool exito_al_inicializar_hilo = false;

    //log_debug(log_kernel_gral, "Hilo responsable de cola NEW listo.");

    pcb = list_remove(cola_new, 0);

    //exito_al_inicializar_proceso = enviar_nuevo_proceso_a_memoria();
    if(exito_al_inicializar_proceso) {
        // crear tcb
        //exito_al_inicializar_hilo = enviar_nuevo_hilo_a_memoria();
    }
    //.
    //.



    //list_add(cola_ready, tcb);

    return NULL;

/* --- COPIADO DE TP ANTERIOR. PARA TENER DE REFERENCIA. ---

    t_pcb* pcb = NULL;
    bool exito = true;

    log_debug(log_kernel_gral, "Hilo responsable de cola NEW listo.");

    while (1) {

        if (exito) {
            sem_wait(&sem_procesos_new);
        }

        pthread_mutex_lock(&mutex_grado_multiprogramacion);
        pthread_mutex_lock(&mutex_procesos_activos);

        if (procesos_activos < grado_multiprogramacion) {
            pthread_mutex_lock(&mutex_cola_new);
            pthread_mutex_lock(&mutex_cola_ready);

            pcb = list_remove(cola_new, 0);
            list_add(cola_ready, pcb);
            procesos_activos++;
            sem_post(&sem_procesos_ready);
            log_info(log_kernel_oblig, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->pid); // log Obligatorio

            pthread_mutex_unlock(&mutex_cola_ready);
            pthread_mutex_unlock(&mutex_cola_new);
            pthread_mutex_unlock(&mutex_procesos_activos);
            pthread_mutex_unlock(&mutex_grado_multiprogramacion);
        }
        else {
            bool exito = false;
            log_error(log_kernel_gral, "No se pudo activar proceso por estar lleno el grado de multiprogramacion.");
            pthread_mutex_unlock(&mutex_procesos_activos);
            pthread_mutex_unlock(&mutex_grado_multiprogramacion);
            
            log_debug(log_kernel_gral, "Volviendo a intentar activar proceso...");
        }

    }
*/

}

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

bool enviar_nuevo_proceso_a_memoria() {

    bool exito_al_crear_proceso_en_memoria = false;

    int socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    //enviar_nuevo_proceso(socket_memoria);

    char* respuesta;
    //respuesta = recibir_mensaje(socket_memoria);

    liberar_conexion(log_kernel_gral, "Memoria", socket_memoria);

    return exito_al_crear_proceso_en_memoria;
}

bool enviar_nuevo_hilo_a_memoria() {
    //
}

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================


// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void enviar_nuevo_proceso(int socket) {
    t_paquete* paquete;
    //crear_paquete(paquete);
    //agregar_a_paquete(paquete, zzzzz);
    //enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}
