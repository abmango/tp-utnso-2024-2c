#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <string.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <utils/general.h>
#include <utils/conexiones.h>
#include <pthread.h>

// ==========================================================================
// ====  Variables globales:  ===============================================
// ==========================================================================

extern int socket_escucha_puerto_dispatch;
extern int socket_escucha_puerto_interrupt;
extern int socket_kernel_dispatch;
extern int socket_kernel_interrupt;
extern int socket_memoria;

extern t_log* log_cpu_oblig; // logger para los logs obligatorios
extern t_log* log_cpu_gral; // logger para los logs nuestros. Loguear con criterio de niveles.

extern t_config* config;

typedef enum {
    DESCONOCIDA,

    // instrucciones (solo cpu)
    SET, // (registro, valor)
    READ_MEM, // (registro dat, registro dir)
    WRITE_MEM, // (registro dir, registro dat)
    SUM, // (registro, registro)
    SUB, // (registro, registro)
    JNZ, // (registro, instruccion)
    LOG, // registro

    // syscalls (guardar contexto y dar control a kernel)
    DUMP_MEMORY, 
    IO, // (tiempo)
    PROCESS_CREATE, // (archivo, tamaño, prioridad tid 0)
    THREAD_CREATE, // (archivo, prioridad)
    THREAD_JOIN, // (tid)
    THREAD_CANCEL, // (tid)
    MUTEX_CREATE, // (recurso)
    MUTEX_LOCK, // (recurso)
    MUTEX_UNLOCK, // (recurso)
    THREAD_EXIT,
    PROCESS_EXIT
} execute_op_code;

typedef struct {
    int pid;
    int tid;
    uint32_t PC;
    t_reg_cpu registros;
    unsigned int base;
    unsigned int limite;
} t_contexto_exec;

typedef enum {
    NINGUNA,
    DESALOJO,
    SYSCALL,
    SEG_FAULT
} t_interrupcion;

extern t_contexto_exec contexto_exec;
extern t_interrupcion tipo_interrupcion;
extern pthread_mutex_t mutex_interrupcion;

// ==========================================================================
// ====  Funciones Internas:  ===============================================
// ==========================================================================

t_list* decode (char* instruccion); // carga en lista la intruccion

// intrucciones para facilitar implementacion solo pasarles directamente lo decodificado (sin el op_code)
void instruccion_set (t_list* param);
void instruccion_read_mem (t_list* param);
void instruccion_write_mem (t_list* param);
void instruccion_sum (t_list* param);
void instruccion_sub (t_list* param);
void instruccion_jnz (t_list* param);
void instruccion_log (t_list* param);

// ==========================================================================
// ====  Funciones Externas:  ===============================================
// ==========================================================================

char* fetch (void);

// syscalls para facilitar implementacion solo pasarles directamente lo decodificado (sin el op_code)
void syscall_dump_memory (void);
void syscall_io (t_list* param);
void syscall_process_create (t_list* param);
void syscall_thread_create (t_list* param);
void syscall_thread_join (t_list* param);
void syscall_thread_cancel (t_list* param);
void syscall_mutex_create (t_list* param);
void syscall_mutex_lock (t_list* param);
void syscall_mutex_unlock (t_list* param);
void syscall_thread_exit (void);
void syscall_process_exit (void);

// ==========================================================================
// ====  Funciones Auxiliares:  =============================================
// ==========================================================================

void iniciar_logs(bool testeo);
void terminar_programa(); // revisar tema socket_kernel...

#endif /* UTILS_CPU_H_ */