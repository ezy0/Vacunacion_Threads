#include <stdio.h>
#include <pthread.h>

// Declaración de variables globales
int numHabitantes; // Número de habitantes a vacunar
int numCentros; // Número de centros de vacunación
int numFarmacias; // Número de farmacias fabricantes de vacunas

// Declaración de funciones
void *vacunarHabitante(void *arg); // Función que ejecutará cada thread para vacunar a un habitante
void *fabricarVacuna(void *arg); // Función que ejecutará cada thread para fabricar una vacuna

int main(int argc, char *argv[]) {
  // Inicialización de variables
  numHabitantes = 100;
  numCentros = 5;
  numFarmacias = 3;

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
