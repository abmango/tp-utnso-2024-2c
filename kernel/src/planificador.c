#include "planificador.h"
#include "quantum.h"
#include "respuesta_memory_dump.h"
#include <limits.h>
#include "io.h"

// ==========================================================================
// ====  Variables globales (exclusivas del planificador):  =================
// ==========================================================================

int contador_pid = 0;

// ==========================================================================
// ====  Función principal (que inicia el planificador):  ===================
// ==========================================================================

void iniciar_planificador(void) {

    log_debug(log_kernel_gral, "Planificador corto plazo iniciado.");

    // Dado que FIFO y prioridades ya tienen sus colas ordenadas según algoritmo, pueden llamar al mismo planificador ya que el comportamiento es el mismo
    if (cod_algoritmo_planif_corto == FIFO || cod_algoritmo_planif_corto == PRIORIDADES) {
        planific_corto_fifo_y_prioridades();
    }
    else if (cod_algoritmo_planif_corto == CMN) {
        planific_corto_multinivel_rr();
    }
    
}

// ==================================================
// -----------------------------------------------
// ---  ALGORITMOS  ------------------------------
// -----------------------------------------------
// ==================================================

/**
 * Se determinó juntar ambos algoritmos en una sola función, ya que la única diferencia
 * entre los algoritmos la manejamos con la variable puntero a función "ingresar_a_ready".
 */ 
void planific_corto_fifo_y_prioridades(void) {
    bool plani_puede_proseguir = true;
    int codigo_recibido = -1;
    // Lista con data del paquete recibido desde cpu.
    t_list* argumentos_recibidos = NULL;
    // variables definidas acá porque se repiten en varios case del switch
    t_pcb* pcb = NULL;
    t_tcb* tcb = NULL;
    int* pid = NULL;
    int* tid = NULL;
    char* path_instrucciones = NULL;
    int* tamanio = NULL;
    int* prioridad = NULL;
    char* nombre_mutex = NULL;
    t_mutex* mutex_encontrado = NULL;

    if(cod_algoritmo_planif_corto == FIFO) {
        log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo FIFO.");
    }
    if(cod_algoritmo_planif_corto == PRIORIDADES) {
        log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo PRIORIDADES.");
    }

    sem_wait(&sem_cola_ready_unica);

    pthread_mutex_lock(&mutex_cola_ready_unica);
    ejecutar_siguiente_hilo(cola_ready_unica);
    pthread_mutex_unlock(&mutex_cola_ready_unica);

    while(true) {

        // Se queda esperando alguna Syscall del hilo en ejecución
        argumentos_recibidos = recibir_de_cpu(&codigo_recibido);

        // Switch para tratar los códigos recibidos
		switch (codigo_recibido) {
            case SYSCALL_MEMORY_DUMP:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: DUMP_MEMORY", hilo_exec->pid_pertenencia, hilo_exec->tid);
            enviar_pedido_de_dump_a_memoria(hilo_exec);
            break;

            case SYSCALL_IO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: IO", hilo_exec->pid_pertenencia, hilo_exec->tid);
            int* unidades_de_trabajo = list_get(argumentos_recibidos, 0);
            usar_io(hilo_exec, *unidades_de_trabajo);
            break;

            case SYSCALL_CREAR_PROCESO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: PROCESS_CREATE", hilo_exec->pid_pertenencia, hilo_exec->tid);
            path_instrucciones = string_duplicate(list_get(argumentos_recibidos, 0));
            tamanio = list_get(argumentos_recibidos, 1);
            prioridad = list_get(argumentos_recibidos, 2);
            pcb = nuevo_proceso(*tamanio, *prioridad, path_instrucciones);
            log_info(log_kernel_oblig, "## (%d:0) Se crea el proceso - Estado: NEW", pcb->pid);
            // NEW se ocupa de enviar el nuevo proceso a Memoria.
            pthread_mutex_lock(&mutex_cola_new);
            ingresar_a_new(pcb);
            pthread_mutex_unlock(&mutex_cola_new);
            break;

            case SYSCALL_CREAR_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_CREATE", hilo_exec->pid_pertenencia, hilo_exec->tid);
            path_instrucciones = string_duplicate(list_get(argumentos_recibidos, 0));
            prioridad = list_get(argumentos_recibidos, 1);
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            tcb = nuevo_hilo(pcb, *prioridad, path_instrucciones);
            enviar_nuevo_hilo_a_memoria(tcb);
            log_info(log_kernel_oblig, "## (%d:%d) Se crea el Hilo - Estado: READY", tcb->pid_pertenencia, tcb->tid);
            ingresar_a_ready(tcb);
            break;

            case SYSCALL_JOIN_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_JOIN", hilo_exec->pid_pertenencia, hilo_exec->tid);
            tid = list_get(argumentos_recibidos, 0);
            hacer_join(hilo_exec, *tid);
            break;

            case SYSCALL_FINALIZAR_ALGUN_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_CANCEL", hilo_exec->pid_pertenencia, hilo_exec->tid);
            tid = list_get(argumentos_recibidos, 0);
            tcb = encontrar_y_remover_tcb(hilo_exec->pid_pertenencia, *tid);
            if(tcb != NULL) {
                finalizar_hilo(tcb);
            }
            break;

            case SYSCALL_FINALIZAR_ESTE_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_EXIT", hilo_exec->pid_pertenencia, hilo_exec->tid);
            finalizar_hilo(hilo_exec);
            break;

            case SYSCALL_CREAR_MUTEX:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: MUTEX_CREATE", hilo_exec->pid_pertenencia, hilo_exec->tid);
            nombre_mutex = string_duplicate(list_get(argumentos_recibidos, 0));
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            if(ya_existe_mutex(pcb, nombre_mutex)){
                log_debug(log_kernel_gral, "Ya existe un Mutex llamado %s !!!", nombre_mutex);
                free(nombre_mutex);
            }
            else {
                crear_mutex(pcb, nombre_mutex);
            }
            break;

            case SYSCALL_BLOQUEAR_MUTEX:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: MUTEX_LOCK", hilo_exec->pid_pertenencia, hilo_exec->tid);
            nombre_mutex = list_get(argumentos_recibidos, 0);
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            mutex_encontrado = encontrar_mutex(pcb, nombre_mutex);
            if(mutex_encontrado != NULL){
                bloquear_mutex(hilo_exec, mutex_encontrado);
            }
            break;

            case SYSCALL_DESBLOQUEAR_MUTEX:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: MUTEX_UNLOCK", hilo_exec->pid_pertenencia, hilo_exec->tid);
            nombre_mutex = list_get(argumentos_recibidos, 0);
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            mutex_encontrado = encontrar_mutex(pcb, nombre_mutex);
            if(mutex_encontrado != NULL){
                if(mutex_esta_asignado_a_hilo(mutex_encontrado, hilo_exec->tid)) {
                    liberar_mutex(mutex_encontrado);
                }
            }
            break;

            case SYSCALL_FINALIZAR_PROCESO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_exec->pid_pertenencia, hilo_exec->tid);
            finalizar_proceso();
            break;
           
            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", hilo_exec->pid_pertenencia);
            break;
		}

        // Si el pthread de de rta_memory_dump está esperando para finalizar
        // un hilo, acá se le permite hacerlo.
        pthread_mutex_lock(&mutex_sincro_rta_memory_dump);
        if(pthread_memory_dump_necesita_finalizar_hilo) {
            pthread_memory_dump_necesita_finalizar_hilo = false;
            plani_puede_proseguir = false;
            sem_post(&sem_sincro_rta_memory_dump);
        }
        pthread_mutex_unlock(&mutex_sincro_rta_memory_dump);

        list_destroy_and_destroy_elements(argumentos_recibidos, (void*)free);

        if(!plani_puede_proseguir) {
            sem_wait(&sem_sincro_rta_memory_dump);
            plani_puede_proseguir = true;
        }

        if(hilo_exec == NULL) { // caso en que el hilo NO continúa ejecutando
            sem_wait(&sem_cola_ready_unica);

            pthread_mutex_lock(&mutex_cola_ready_unica);
            ejecutar_siguiente_hilo(cola_ready_unica);
            pthread_mutex_unlock(&mutex_cola_ready_unica);
        }
        else { // caso en que el hilo continúa ejecutando
            enviar_orden_de_ejecucion_al_cpu(hilo_exec);
        }
	}
}

// La idea es crear una cola nueva de Ready cada vez que llega a Ready un Hilo para cuya
// prioridad no había otros Hilos en Ready. Y eliminar dicha cola cuando no tiene Hilos.
void planific_corto_multinivel_rr(void) {
    bool plani_puede_proseguir = true;
    int codigo_recibido = -1;
    t_list* cola_ready = NULL;
    char* key_cola_ready = NULL;
    // Lista con data del paquete recibido desde cpu.
    t_list* argumentos_recibidos = NULL;
    // variables que defino acá porque las repito en varios case del switch
    t_pcb* pcb = NULL;
    t_tcb* tcb = NULL;
    int* pid = NULL;
    int* tid = NULL;
    char* path_instrucciones = NULL;
    int* tamanio = NULL;
    int* prioridad = NULL;
    char* nombre_mutex = NULL;
    t_mutex* mutex_encontrado = NULL;

    log_debug(log_kernel_gral, "Planificador corto plazo listo para funcionar con algoritmo CMN y Round Robin.");

    sem_wait(&sem_hilos_ready_en_cmn);

    pthread_mutex_lock(&mutex_colas_ready_cmn);
    cola_ready = obtener_cola_ready_cmn(0);
    ejecutar_siguiente_hilo(cola_ready);
    // se destruye la cola ready de prioridad 0, pues ahora no tiene hilos.
    dictionary_remove(diccionario_ready_multinivel, "0");
    list_destroy(cola_ready);
    pthread_mutex_unlock(&mutex_colas_ready_cmn);

    while (true) {

        // Se queda esperando alguna Syscall del hilo en ejecución, o su interrupción por Quantum agotado.
        argumentos_recibidos = esperar_cpu_rr(&codigo_recibido);

        // Switch para tratar los códigos recibidos
        switch (codigo_recibido) {
            case SYSCALL_MEMORY_DUMP:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: DUMP_MEMORY", hilo_exec->pid_pertenencia, hilo_exec->tid);
            enviar_pedido_de_dump_a_memoria(hilo_exec);
            break;

            case SYSCALL_IO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: IO", hilo_exec->pid_pertenencia, hilo_exec->tid);
            int* unidades_de_trabajo = list_get(argumentos_recibidos, 0);
            usar_io(hilo_exec, *unidades_de_trabajo);
            break;

            case SYSCALL_CREAR_PROCESO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: PROCESS_CREATE", hilo_exec->pid_pertenencia, hilo_exec->tid);
            path_instrucciones = string_duplicate(list_get(argumentos_recibidos, 0));
            tamanio = list_get(argumentos_recibidos, 1);
            prioridad = list_get(argumentos_recibidos, 2);
            pcb = nuevo_proceso(*tamanio, *prioridad, path_instrucciones);
            log_info(log_kernel_oblig, "## (%d:0) Se crea el proceso - Estado: NEW", pcb->pid);
            // NEW se ocupa de enviar el nuevo proceso a Memoria.
            pthread_mutex_lock(&mutex_cola_new);
            ingresar_a_new(pcb);
            pthread_mutex_unlock(&mutex_cola_new);
            break;

            case SYSCALL_CREAR_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_CREATE", hilo_exec->pid_pertenencia, hilo_exec->tid);
            path_instrucciones = string_duplicate(list_get(argumentos_recibidos, 0));
            prioridad = list_get(argumentos_recibidos, 1);
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            tcb = nuevo_hilo(pcb, *prioridad, path_instrucciones);
            enviar_nuevo_hilo_a_memoria(tcb);
            log_info(log_kernel_oblig, "## (%d:%d) Se crea el Hilo - Estado: READY", tcb->pid_pertenencia, tcb->tid);
            ingresar_a_ready(tcb);
            break;

            case SYSCALL_JOIN_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_JOIN", hilo_exec->pid_pertenencia, hilo_exec->tid);
            tid = list_get(argumentos_recibidos, 0);
            hacer_join(hilo_exec, *tid);
            break;

            case SYSCALL_FINALIZAR_ALGUN_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_CANCEL", hilo_exec->pid_pertenencia, hilo_exec->tid);
            tid = list_get(argumentos_recibidos, 0);
            tcb = encontrar_y_remover_tcb(hilo_exec->pid_pertenencia, *tid);
            if(tcb != NULL) {
                finalizar_hilo(tcb);
            }
            break;

            case SYSCALL_FINALIZAR_ESTE_HILO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: THREAD_EXIT", hilo_exec->pid_pertenencia, hilo_exec->tid);
            finalizar_hilo(hilo_exec);
            break;

            case SYSCALL_CREAR_MUTEX:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: MUTEX_CREATE", hilo_exec->pid_pertenencia, hilo_exec->tid);
            nombre_mutex = string_duplicate(list_get(argumentos_recibidos, 0));
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            if(ya_existe_mutex(pcb, nombre_mutex)){
                log_debug(log_kernel_gral, "Ya existe un Mutex llamado %s !!!", nombre_mutex);
                free(nombre_mutex);
            }
            else {
                crear_mutex(pcb, nombre_mutex);
            }
            break;

            case SYSCALL_BLOQUEAR_MUTEX:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: MUTEX_LOCK", hilo_exec->pid_pertenencia, hilo_exec->tid);
            nombre_mutex = list_get(argumentos_recibidos, 0);
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            mutex_encontrado = encontrar_mutex(pcb, nombre_mutex);
            if(mutex_encontrado != NULL){
                bloquear_mutex(hilo_exec, mutex_encontrado);
            }
            break;

            case SYSCALL_DESBLOQUEAR_MUTEX:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: MUTEX_UNLOCK", hilo_exec->pid_pertenencia, hilo_exec->tid);
            nombre_mutex = list_get(argumentos_recibidos, 0);
            pcb = encontrar_pcb_activo(hilo_exec->pid_pertenencia);
            mutex_encontrado = encontrar_mutex(pcb, nombre_mutex);
            if(mutex_encontrado != NULL){
                if(mutex_esta_asignado_a_hilo(mutex_encontrado, hilo_exec->tid)) {
                    liberar_mutex(mutex_encontrado);
                }
            }
            break;

            case SYSCALL_FINALIZAR_PROCESO:
            log_info(log_kernel_oblig, "## (%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_exec->pid_pertenencia, hilo_exec->tid);
            finalizar_proceso();
            break;

            case INTERRUPCION:
            log_info(log_kernel_oblig, "## (%d:%d) - Desalojado por fin de Quantum", hilo_exec->pid_pertenencia, hilo_exec->tid);
            ingresar_a_ready(hilo_exec);
            hilo_exec = NULL;
            break;

            default:
            log_error(log_kernel_gral, "El motivo de desalojo del proceso %d no se puede interpretar, es desconocido.", hilo_exec->pid_pertenencia);
            break;
        }

        // Si el pthread de de rta_memory_dump está esperando para finalizar
        // un hilo, acá se le permite hacerlo.
        pthread_mutex_lock(&mutex_sincro_rta_memory_dump);
        if(pthread_memory_dump_necesita_finalizar_hilo) {
            pthread_memory_dump_necesita_finalizar_hilo = false;
            plani_puede_proseguir = false;
            sem_post(&sem_sincro_rta_memory_dump);
        }
        pthread_mutex_unlock(&mutex_sincro_rta_memory_dump);

        list_destroy_and_destroy_elements(argumentos_recibidos, (void*)free);

        if(!plani_puede_proseguir) {
            sem_wait(&sem_sincro_rta_memory_dump);
            plani_puede_proseguir = true;
        }

        if(hilo_exec == NULL) { // caso en que el hilo NO continúa ejecutando
            reiniciar_quantum();

            sem_wait(&sem_hilos_ready_en_cmn);

            pthread_mutex_lock(&mutex_colas_ready_cmn);
            cola_ready = encontrar_cola_multinivel_de_mas_baja_prioridad(&key_cola_ready);
            ejecutar_siguiente_hilo(cola_ready);
            if(list_is_empty(cola_ready)) { // si no quedan hilos en esa cola, se destruye.
                dictionary_remove(diccionario_ready_multinivel, key_cola_ready);
                list_destroy(cola_ready);
            }
            free(key_cola_ready);
            pthread_mutex_unlock(&mutex_colas_ready_cmn);
        }
        else { // caso en que el hilo continúa ejecutando
            enviar_orden_de_ejecucion_al_cpu(hilo_exec);
        }
    }
}


// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

void ejecutar_siguiente_hilo(t_list* cola_ready) {
    hilo_exec = list_remove(cola_ready, 0);
    enviar_orden_de_ejecucion_al_cpu(hilo_exec);
}

t_list* recibir_de_cpu(int* codigo_operacion) {
    t_list* argumentos_recibidos = NULL;
    *codigo_operacion = recibir_codigo(socket_cpu_dispatch);
    if (*codigo_operacion >= 0) {
        argumentos_recibidos = recibir_paquete(socket_cpu_dispatch);
    }
    else {
        log_error(log_kernel_gral, "No se pudo recibir el codigo de operacion enviado por CPU.");
    }
    return argumentos_recibidos;
}

t_pcb* encontrar_pcb_activo(int pid) {
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&mutex_procesos_activos);
    pcb = buscar_pcb_por_pid(procesos_activos, pid);
    pthread_mutex_unlock(&mutex_procesos_activos);
    if(pcb == NULL) log_debug(log_kernel_gral, "No se encontro pcb activo");
    return pcb;
}

t_tcb* encontrar_y_remover_tcb(int pid, int tid) {
    t_tcb* tcb = NULL;
    // Busca en EXEC
    if(hilo_exec != NULL) {
        if((hilo_exec->tid == tid) && (hilo_exec->pid_pertenencia == pid)) {
            tcb = hilo_exec;
            hilo_exec = NULL;
            return tcb;
        }
    }
    // Busca en BLOCKED (esperando por IO)
    pthread_mutex_lock(&mutex_cola_blocked_io);
    tcb = buscar_tcb_por_pid_y_tid(cola_blocked_io, pid, tid);
    if(tcb != NULL) {
        list_remove_element(cola_blocked_io, tcb);
        return tcb;
    }
    pthread_mutex_unlock(&mutex_cola_blocked_io);
    // Busca en BLOCKED (usando IO)
    pthread_mutex_lock(&mutex_hilo_usando_io);
    if(hilo_usando_io != NULL) {
        if((hilo_usando_io->tid == tid) && (hilo_usando_io->pid_pertenencia == pid)) {
            tcb = hilo_usando_io;
            hilo_usando_io = NULL;
            return tcb;
        }
    }
    pthread_mutex_unlock(&mutex_hilo_usando_io);
    // Busca en BLOCKED (joineados)
    tcb = buscar_tcb_por_pid_y_tid(cola_blocked_join, pid, tid);
    if(tcb != NULL) {
        list_remove_element(cola_blocked_join, tcb);
        return tcb;
    }
    // Busca en BLOCKED (esperando respuesta Memory Dump)
    pthread_mutex_lock(&mutex_cola_blocked_memory_dump);
    tcb = buscar_tcb_por_pid_y_tid(cola_blocked_memory_dump, pid, tid);
    if(tcb != NULL) {
        list_remove_element(cola_blocked_memory_dump, tcb);
        return tcb;
    }
    pthread_mutex_unlock(&mutex_cola_blocked_memory_dump);
    // Busca en READY
    tcb = encontrar_y_remover_tcb_en_ready(pid, tid);
    return tcb;
}

void finalizar_hilo(t_tcb* tcb) {
    t_pcb* pcb = encontrar_pcb_activo(tcb->pid_pertenencia);
	if(tcb->tid == 0) { // (if es Hilo main)
        finalizar_hilos_no_main_de_proceso(pcb);
        pthread_mutex_lock(&mutex_procesos_activos);
        list_remove_element(procesos_activos, pcb);
        pthread_mutex_unlock(&mutex_procesos_activos);
        pthread_mutex_lock(&mutex_procesos_exit);
        list_add(procesos_exit, pcb);
        pthread_mutex_unlock(&mutex_procesos_exit);
	}
    liberar_hilo(pcb, tcb);
    log_info(log_kernel_oblig, "## (%d:%d) Finaliza el hilo", tcb->pid_pertenencia, tcb->tid);
    if(tcb->tid == 0) { // (if es Hilo main)
        log_info(log_kernel_oblig, "## Finaliza el proceso %d", tcb->pid_pertenencia);
    }
    if(hilo_exec == tcb) {
        hilo_exec = NULL;
    }
    pthread_mutex_lock(&mutex_cola_exit);
    mandar_a_exit(tcb);
    pthread_mutex_unlock(&mutex_cola_exit);
}

void finalizar_proceso(void) {

    int pid_proceso = hilo_exec->pid_pertenencia;
    if(hilo_exec->tid == 0) {
        finalizar_hilo(hilo_exec);
    }
    else {
        t_pcb* pcb = encontrar_pcb_activo(pid_proceso);
        finalizar_hilo(pcb->hilo_main);
    }
}

void crear_mutex(t_pcb* pcb, char* nombre) {
    t_mutex* nuevo_mutex = malloc(sizeof(t_mutex));
    nuevo_mutex->nombre = nombre;
    nuevo_mutex->asignado = false;
    nuevo_mutex->tid_asignado = -1;
    nuevo_mutex->bloqueados_esperando = list_create();
    list_add(pcb->mutex_creados, nuevo_mutex);
}

bool ya_existe_mutex(t_pcb* pcb, char* nombre) {

    bool _mutex_tiene_el_mismo_nombre(t_mutex* mutex) {
        return strcmp(mutex->nombre, nombre) == 0;
    }

    return list_any_satisfy(pcb->mutex_creados, (void*)_mutex_tiene_el_mismo_nombre);
}

t_mutex* encontrar_mutex(t_pcb* pcb, char* nombre) {

    bool _el_mutex_tiene_el_mismo_nombre(t_mutex* mutex) {
        return strcmp(mutex->nombre, nombre) == 0;
    }

    t_mutex* mutex_encontrado = NULL;
    mutex_encontrado = list_find(pcb->mutex_creados, (void*)_el_mutex_tiene_el_mismo_nombre);
    return mutex_encontrado;
}

t_list* obtener_cola_ready_cmn(int prioridad) {
    t_list* cola_ready = NULL;
    char* prio = string_itoa(prioridad); //para guardar referencia
    cola_ready = dictionary_get(diccionario_ready_multinivel, prio);
    free(prio);
    return cola_ready;
}

bool mutex_esta_asignado_a_hilo(t_mutex* mutex, int tid) {
    return (mutex->asignado == true) && (mutex->tid_asignado == tid);
}

void enviar_nuevo_hilo_a_memoria(t_tcb* tcb) {
    int socket_memoria = crear_conexion_memoria(); // considera el handshake
    enviar_nuevo_hilo(tcb, socket_memoria);
    recibir_mensaje_de_rta(log_kernel_gral, "CREAR HILO", socket_memoria);
    liberar_conexion(log_kernel_gral, "Memoria (por Creacion de Hilo)", socket_memoria);
}

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

t_pcb* nuevo_proceso(int tamanio, int prioridad_hilo_main, char* path_instruc_hilo_main) {
    t_pcb* nuevo_pcb = crear_pcb(contador_pid, tamanio);
    contador_pid++;
    t_tcb* nuevo_tcb = crear_tcb(nuevo_pcb->pid, nuevo_pcb->sig_tid_a_asignar, prioridad_hilo_main, path_instruc_hilo_main);
    nuevo_pcb->sig_tid_a_asignar++;
    nuevo_pcb->hilo_main = nuevo_tcb;
    return nuevo_pcb;
}

void ingresar_a_new(t_pcb* pcb) {
    list_add(cola_new, pcb);
    sem_post(&sem_cola_new);
}

t_tcb* nuevo_hilo(t_pcb* pcb_creador, int prioridad, char* path_instrucciones) {
    t_tcb* nuevo_tcb = crear_tcb(pcb_creador->pid, pcb_creador->sig_tid_a_asignar, prioridad, path_instrucciones);
    pcb_creador->sig_tid_a_asignar++;
    asociar_tid(pcb_creador, nuevo_tcb);
    return nuevo_tcb;
}

void bloquear_mutex(t_tcb* tcb, t_mutex* mutex) {
    if(mutex->asignado == true) {
        // Manda el hilo a la cola de bloqueados esperando por el mutex
        list_add(mutex->bloqueados_esperando, tcb);
        log_info(log_kernel_oblig, "## (%d:%d) - Bloqueado por: MUTEX", tcb->pid_pertenencia, tcb->tid);
        hilo_exec = NULL;
        return;
    }
    // si el mutex está libre
    mutex->asignado = true;
    mutex->tid_asignado = tcb->tid;
    log_debug(log_kernel_gral, "Mutex %s asignado a ## (%d:%d)", mutex->nombre, tcb->pid_pertenencia, tcb->tid);
}

void liberar_mutex(t_mutex* mutex) {
    mutex->asignado = false;
    mutex->tid_asignado = -1;
    log_debug(log_kernel_gral, "Mutex %s liberado", mutex->nombre);
    if(!list_is_empty(mutex->bloqueados_esperando)) {
        t_tcb* tcb = list_remove(mutex->bloqueados_esperando, 0);
        mutex->asignado = true;
        mutex->tid_asignado = tcb->tid;
        ingresar_a_ready(tcb);
        log_debug(log_kernel_gral, "## (%d:%d) desbloqueado. Mutex %s asignado. Pasa a READY", tcb->pid_pertenencia, tcb->tid, mutex->nombre);
    }
}

void liberar_joineado(t_tcb* tcb) {
    tcb->tid_joined = -1;
    ingresar_a_ready(tcb);
    log_debug(log_kernel_gral, "## (%d:%d) desbloqueado (fin de hilo joineado). Pasa a READY", tcb->pid_pertenencia, tcb->tid);
}

void hacer_join(t_tcb* tcb, int tid_a_joinear) {

	bool _es_el_tid_buscado_para_joinear(int* tid) {
		return (*tid) == tid_a_joinear;
	}

    t_pcb* pcb = encontrar_pcb_activo(tcb->pid_pertenencia);
    if(tid_a_joinear == 0 || list_any_satisfy(pcb->tids_asociados, (void*)_es_el_tid_buscado_para_joinear)) {
        tcb->tid_joined = tid_a_joinear;
        list_add(cola_blocked_join, tcb);
        log_info(log_kernel_oblig, "## (%d:%d) - Bloqueado por: PTHREAD_JOIN", tcb->pid_pertenencia, tcb->tid);
        hilo_exec = NULL;
    }
    else {
        log_debug(log_kernel_gral, "TID %d a joinear no encontrado", tid_a_joinear);
    }
}

void enviar_orden_de_ejecucion_al_cpu(t_tcb* tcb) {
    t_paquete* paquete = crear_paquete(EJECUCION);
    agregar_a_paquete(paquete, (void*)&(tcb->pid_pertenencia), sizeof(int));
    agregar_a_paquete(paquete, (void*)&(tcb->tid), sizeof(int));
    enviar_paquete(paquete, socket_cpu_dispatch);
    log_debug(log_kernel_gral, "## (%d:%d) - EJECUTANDO", tcb->pid_pertenencia, tcb->tid);
    eliminar_paquete(paquete);
}

void enviar_pedido_de_dump_a_memoria(t_tcb* tcb) {
    int socket_memoria = crear_conexion_memoria(); // considera el handshake
    enviar_pedido_de_dump(tcb->pid_pertenencia, tcb->tid, socket_memoria);
    pthread_mutex_lock(&mutex_cola_blocked_memory_dump);
    list_add(cola_blocked_memory_dump, tcb);
    pthread_mutex_unlock(&mutex_cola_blocked_memory_dump);
    log_info(log_kernel_oblig, "## (%d:%d) - Bloqueado por: MEMORY_DUMP", tcb->pid_pertenencia, tcb->tid);
    hilo_exec = NULL;

    t_recepcion_respuesta_memory_dump* info_para_recibir_rta = malloc(sizeof(t_recepcion_respuesta_memory_dump));
    info_para_recibir_rta->tcb = tcb;
    info_para_recibir_rta->socket_de_la_conexion = socket_memoria;
    // acá no debería haber problema de memory leak, pues al terminar el hilo detacheado, lo liberaría.
    pthread_t thread_respuesta_memory_dump;
    pthread_create(&thread_respuesta_memory_dump, NULL, rutina_respuesta_memory_dump, (void*)info_para_recibir_rta);
    pthread_detach(&thread_respuesta_memory_dump);
}

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

t_pcb* crear_pcb(int pid, int tamanio) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->tamanio = tamanio;
    pcb->tids_asociados = list_create();
    pcb->mutex_creados = list_create();
    pcb->sig_tid_a_asignar = 0;
    pcb->hilo_main = NULL;
    return pcb;
}

void enviar_pedido_de_dump(int pid, int tid, int socket) {
    t_paquete* paquete = crear_paquete(MEMORY_DUMP);
    agregar_a_paquete(paquete, (void*)&pid, sizeof(int));
    agregar_a_paquete(paquete, (void*)&tid, sizeof(int));
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}
