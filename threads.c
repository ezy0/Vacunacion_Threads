#include <stdio.h>
#include <pthread.h>

// Declaración de variables globales que se leen del fichero
int numHabitantes; 
int numVacunasIniciales;
int minVacunasPorTanda;
int maxVacunasPorTanda;
int minTiempoFabricacion;
int maxTiempoFabricacion;
int minTiempoReparto;
int maxTiempoReaccion;
int minTiempoDesplazamiento;
int maxTiempoDesplazamiento;

// Declaración de funciones
void *vacunarHabitante(void *arg); // Función que ejecutará cada thread para vacunar a un habitante
void *fabricarVacuna(void *arg); // Función que ejecutará cada thread para fabricar una vacuna

int main(int argc, char *argv[]) {
  // declaracion e inicialización de variables que no se leen del fichero
  int numCentros = 5;
  int numFarmacias = 3;

  // Creación de threads para vacunar a cada habitante
  pthread_t threads[numHabitantes];
  for (int i = 0; i < numHabitantes; i++) {
    int *arg = malloc(sizeof(*arg));
    *arg = i;
    pthread_create(&threads[i], NULL, vacunarHabitante, arg);
  }

  // Creación de threads para fabricar cada vacuna
  pthread_t threadsF[numFarmacias];
  for (int i = 0; i < numFarmacias; i++) {
    int *arg = malloc(sizeof(*arg));
    *arg = i;
    pthread_create(&threadsF[i], NULL, fabricarVacuna, arg);
  }

  // Espera a que todos los threads hayan terminado
  for (int i = 0; i < numHabitantes; i++) {
    pthread_join(threads[i], NULL);
  }
  for (int i = 0; i < numFarmacias; i++) {
    pthread_join(threadsF[i], NULL);
  }

  return 0;
}

// Función que ejecutará cada thread para vacunar a un habitante
void *vacunarHabitante(void *arg) {
  int id = *(int *)arg;
  free(arg);

  // Código para vacunar al habitante con el ID especificado

  return NULL;
}

// Función que ejecutará cada thread para fabricar una vacuna
void *fabricarVacuna(void *arg) {
  int id = *(int *)arg;
  free(arg);

  // Código para fabricar una vacuna en la farmacia con el ID especificado

  return NULL;
}


/* Lectura de datos del fichero

  // Abrir el fichero de texto
  FILE *fp = fopen("valores.txt", "r");

  // Leer los valores del fichero y asignarlos a las variables
  fscanf(fp, "%d", &numHabitantes);
  fscanf(fp, "%d", &numVacunasIniciales);
  fscanf(fp, "%d", &minVacunasPorTanda);
  fscanf(fp, "%d", &maxVacunasPorTanda);
  fscanf(fp, "%d", &minTiempoFabricacion);
  fscanf(fp, "%d", &maxTiempoFabricacion);
  fscanf(fp, "%d", &maxTiempoReparto);
  fscanf(fp, "%d", &maxTiempoReaccion);
  fscanf(fp, "%d", &maxTiempoDesplazamiento);

  // Cerrar el fichero
  fclose(fp);

*/
