#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Declaración de variables globales que se leen del fichero
int	numHabitantes; 
int	numVacunasIniciales;
int	minVacunasPorTanda;
int	maxVacunasPorTanda;
int	minTiempoFabricacion;
int	maxTiempoFabricacion;
int	minTiempoReparto = 1;
int	maxTiempoReparto;
int	minTiempoReaccion = 1;
int	maxTiempoReaccion;
int	minTiempoDesplazamiento = 1;
int	maxTiempoDesplazamiento;

// Variables globales ???
int	numFarmacias = 3;
int	maxVacunas = 400;

// Array de vacunas por centro
int	vacunas[5];

// Creacion del mutex de las vacunas por centro y de las condiciones
pthread_mutex_t mutex_vacunas;
pthread_cond_t disponible, no_disponible;

// Declaración del fichero por donde saldrá el resultado final
FILE	*salida;

// Declaración de funciones
void	*vacunarHabitante(void *arg); // Función que ejecutará cada thread para vacunar a un habitante
void	leerFichero(char *fichero); //Damos valores a las variables que faltan por inicializar

int	main(int argc, char *argv[]) {
	// Creación de threads para vacunar a cada habitante
	pthread_t	*threads;

	//Asignamos espacio para los threads de los habitantes con malloc
	threads = (pthread_t *)malloc(sizeof(pthread_t) * numHabitantes);

	// Inicializacion del mutex y de las condiciones
	pthread_mutex_init(&mutex_vacunas, NULL);
	pthread_cond_init(&disponible, NULL);
	pthread_cond_init(&no_disponible, NULL);

	// Llamamos a leer fichero para inicializar el resto de variables y declaramos el fichero de salida
	if (argc < 2) {
		leerFichero("valores.txt");
		salida = fopen("salida_vacunacion.txt", "w");
	}
	else if (argc == 2) {
		leerFichero(argv[1]);
		salida = fopen("salida_vacunacion.txt", "w");
	}
	else {
		leerFichero(argv[1]);
		salida = fopen(argv[2], "w");
	}
	if (salida == NULL)
	{
		printf("No se ha podido abrir el archivo para esciribir, se cerrará el programa\n");
		pthread_mutex_destroy(&mutex_vacunas);
		pthread_cond_destroy(&disponible);
		pthread_cond_destroy(&no_disponible);
		free(threads);
		return 1;
	}

	//Repartir vacunas iniciales a cada centro
	for (int i = 0; i < 5; i++)
		vacunas[i] = numVacunasIniciales;
	// Inicialización de threads para vacunar a cada habitante
	for (int i = 0; i < numHabitantes; i++)
		pthread_create(&threads[i], NULL, vacunarHabitante, &i);






	// Espera a que todos los threads hayan terminado
	for (int i = 0; i < numHabitantes; i++)
		pthread_join(threads[i], NULL);

	pthread_mutex_destroy(&mutex_vacunas);
	pthread_cond_destroy(&disponible);
	pthread_cond_destroy(&no_disponible);
	free(threads);
	return 0;
}

// Función que ejecutará cada thread para vacunar a un habitante
void	*vacunarHabitante(void *arg) {
	int num = *(int *)arg;	//numero del habitante
	int	centroAsignado;
	int	waitReaccion, waitDesplazamiento;

	//srand(time(NULL));
	// Esperar a que se entere de la vacunacion
	waitReaccion = rand() % maxTiempoReaccion + 1;
	sleep(waitReaccion);
	
	//Una vez se entera se le asigna el centro con random y escribimos
	centroAsignado = rand() % 5;
	printf("Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);
	fprintf(salida, "Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);

	// Una vez asignado, se espera a que se desplace al centro. En ese momento se hace el lock del mutex
	waitDesplazamiento = rand() % maxTiempoDesplazamiento + 1;
	sleep(waitDesplazamiento);
	pthread_mutex_lock(&mutex_vacunas);

	while (vacunas[centroAsignado] == 0) //Comprobamos si quedan o no vacunas y esperamos en el caso de que no
		pthread_cond_wait(&disponible, &mutex_vacunas);

	vacunas[centroAsignado]--;	// Quitamos una vacuna
	printf("Habitante %d vacunado en el centro %d\n", num, centroAsignado);
	fprintf(salida, "Habitante %d vacunado en el centro %d\n", num, centroAsignado);	

	// Mandamos una señal de que no quedan vacunas, en el caso de que el habitante haya sido la última.
	if (!vacunas[centroAsignado])
		pthread_cond_signal(&no_disponible);
	
	//Unlock del mutex
	pthread_mutex_unlock(&mutex_vacunas);

	return 0;
}


void 	leerFichero(char *fichero) {
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
