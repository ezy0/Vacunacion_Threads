#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// Declaración de variables globales que se leen del fichero
int numHabitantes; 
int numVacunasIniciales;
int minVacunasPorTanda;
int maxVacunasPorTanda;
int minTiempoFabricacion;
int maxTiempoFabricacion;
int	minTiempoReparto = 1;
int maxTiempoReparto;
int minTiempoReaccion = 1;
int maxTiempoReaccion;
int minTiempoDesplazamiento = 1;
int	maxTiempoDesplazamiento;

// Declaración de funciones
void *vacunarHabitante(void *arg); // Función que ejecutará cada thread para vacunar a un habitante
void *fabricarVacuna(void *arg); // Función que ejecutará cada thread para fabricar una vacuna
void leerFichero(char *fichero); //Damos valores a las variables que faltan por inicializar

int main(int argc, char *argv[]) {
	// declaracion e inicialización de variables que no se leen del fichero
	int numCentros = 5;
	int numFarmacias = 3;

	// Creación de threads para vacunar a cada habitante
	pthread_t *threadsH;
	// Creación de threads para fabricar cada vacuna, no necesitaremos malloc porque las farmacias siempre serán 3
	pthread_t threadsF[numFarmacias];

	//Asignamos epacio para los threads de los habitantes con malloc
	threadsH = (pthread_t *)malloc(sizeof(pthread_t) * numHabitantes);

	// Llamamos a leer fichero para inicializar el resto de variables y declaramos las salidas
	if (argc < 2) {
		leerFichero("valores.txt");

	}
	else if (argc == 2) {
		leerFichero(argv[1]);

	}
	else {
		leerFichero(argv[1]);
		
	}
		
	// Inicialización de threads para vacunar a cada habitante
	for (int i = 0; i < numHabitantes; i++)
		pthread_create(&threadsH[i], NULL, vacunarHabitante, &i);

	// Inicilización de threads para fabricar cada vacuna
	for (int i = 0; i < numFarmacias; i++)
		pthread_create(&threadsF[i], NULL, fabricarVacuna, &i);





	// Espera a que todos los threads hayan terminado
	for (int i = 0; i < numHabitantes; i++)
		pthread_join(threadsH[i], NULL);
	for (int i = 0; i < numFarmacias; i++)
		pthread_join(threadsF[i], NULL);

	free(threadsH);
	return 0;
}

// Función que ejecutará cada thread para vacunar a un habitante
void *vacunarHabitante(void *arg) {
	int id = *(int *)arg;

	// Código para vacunar al habitante con el ID especificado

	return 0;
}

// Función que ejecutará cada thread para fabricar una vacuna
void *fabricarVacuna(void *arg) {
	int id = *(int *)arg;

	// Código para fabricar una vacuna en la farmacia con el ID especificado

	return 0;
}

void  leerFichero(char *fichero) {
	// Abrir el fichero de texto
	FILE *fp = fopen(fichero, "r");

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
}
