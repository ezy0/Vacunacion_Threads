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
int	maxTiempoReparto;
int	maxTiempoReaccion;
int	maxTiempoDesplazamiento;

// Array de vacunas por centro
int	vacunas[5];
//Array de habitantes por centro
int demanda[5];
//Array de las vacunas totales que lleva cada fabrica
int	vacunasTotales[3];

// Creacion del mutex de las vacunas por centro y de las condiciones
pthread_mutex_t mutex_vacunas, mutex_fabricas;
pthread_cond_t disponible;
//pthread_cond_t no_disponible;

// Declaración del fichero por donde saldrá el resultado final
FILE	*salida;

// Declaración de funciones
void	*vacunarHabitante(void *arg); // Función que ejecutará cada thread para vacunar a un habitante
void	leerFichero(char *fichero); //Damos valores a las variables que faltan por inicializar
void	*repartirFabrica(void *arg);

int	main(int argc, char *argv[]) {
	// Creación de threads para vacunar a cada habitante
	pthread_t	*threads;
	pthread_t	threadsF[3];
	int			idHabitante = 0;
	int			idHabitanteJoin = 0;
	int			noDemanda;

	//Asignamos espacio para los threads de los habitantes con malloc
	threads = (pthread_t *)malloc(sizeof(pthread_t) * numHabitantes);

	for (int i = 0; i < 5; i++)
		demanda[i] = 0;

	for (int i = 0; i < 3; i++)
		vacunasTotales[i] = 0;

	// Inicializacion del mutex y de las condiciones
	pthread_mutex_init(&mutex_vacunas, NULL);
	pthread_mutex_init(&mutex_fabricas, NULL);
	pthread_cond_init(&disponible, NULL);
	//pthread_cond_init(&no_disponible, NULL);

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
		//pthread_cond_destroy(&no_disponible);
		free(threads);
		return 1;
	}

	//Repartir vacunas iniciales a cada centro
	for (int i = 0; i < 5; i++)
		vacunas[i] = numVacunasIniciales;

	for (int j = 1; j <= 10; j++){
		noDemanda = 0;

		// Inicialización de threads para vacunar a cada habitante
		for (int i = 1; i <= numHabitantes / 10; i++) {
			idHabitante++;
			int	num = idHabitante;
			pthread_create(&threads[idHabitante], NULL, vacunarHabitante, &idHabitante);
			while (num == idHabitante)
				sleep(0.1);
			idHabitante = num;
		}

		// Inicialización de threads para cada fabrica
		for (int i = 1; i <= 3; i++) {
			int	num = i;
			pthread_create(&threadsF[i], NULL, repartirFabrica, &i);
			while (num == i)
				sleep(0.1);
			i = num;
		}

		while (!noDemanda)
			if (demanda[0] == 0 && demanda[1] == 0 && demanda[2] == 0 && demanda[3] == 0 && demanda[4] == 0)
				noDemanda = 1;
		
		// Espera a que todos los threads hayan terminado
		for (int i = 1; i <= 3; i++)
			pthread_join(threadsF[i], NULL);

		// Espera a que todos los threads hayan terminado
		for (int i = 1; i <= numHabitantes / 10; i++) {
			idHabitanteJoin++;
			pthread_join(threads[idHabitanteJoin], NULL);
		}
	}

	pthread_mutex_destroy(&mutex_vacunas);
	pthread_mutex_destroy(&mutex_fabricas);
	pthread_cond_destroy(&disponible);
	//pthread_cond_destroy(&no_disponible);
	free(threads);
	return 0;
}

// Función que ejecutará cada thread para vacunar a un habitante
void	*vacunarHabitante(void *arg) {
	int num = *(int *)arg;	//numero del habitante
	*(int *)arg = 0;	// Sirve de indicador para saber que el numero ya se ha almacenado y no se va a repetir
	int	centroAsignado;
	int	waitReaccion, waitDesplazamiento;

	//srand(time(NULL));
	// Esperar a que se entere de la vacunacion
	waitReaccion = rand() % maxTiempoReaccion + 1;
	sleep(waitReaccion);
	
	//Una vez se entera se le asigna el centro con random y escribimos
	centroAsignado = rand() % 5 + 1;
	printf("Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);
	fprintf(salida, "Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);

	// Una vez asignado, se espera a que se desplace al centro. En ese momento se hace el lock del mutex
	waitDesplazamiento = rand() % maxTiempoDesplazamiento + 1;
	sleep(waitDesplazamiento);
	pthread_mutex_lock(&mutex_vacunas);

	demanda[centroAsignado - 1]++;	//Aumentamos la demanda
	while (vacunas[centroAsignado - 1] == 0)//Comprobamos si quedan o no vacunas y esperamos en el caso de que no
		pthread_cond_wait(&disponible, &mutex_vacunas);

	vacunas[centroAsignado - 1]--;	// Quitamos una vacuna
	demanda[centroAsignado - 1]--;	//Bajamos la demanda

	printf("Habitante %d vacunado en el centro %d\n", num, centroAsignado);
	fprintf(salida, "Habitante %d vacunado en el centro %d\n", num, centroAsignado);

	// Mandamos una señal de que no quedan vacunas, en el caso de que el habitante haya sido la última.
	/*if (!vacunas[centroAsignado - 1])
		pthread_cond_signal(&no_disponible);*/
	
	//Unlock del mutex
	pthread_mutex_unlock(&mutex_vacunas);

	return 0;
}

void	*repartirFabrica(void *arg) {
	int num = *(int *)arg;	//numero de la fabrica
	*(int *)arg = 0;	// Sirve de indicador para saber que el numero ya se ha almacenado y no se va a repetir
	int fabricacionTanda;
	int waitFabricacion, waitReparto;
	int demandaTotal;
	int	vacunasFabricadas;

	while (vacunasTotales[num-1] <= 400) {

		vacunasFabricadas = 0;
		demandaTotal = 0;

		waitFabricacion = rand() % (maxTiempoFabricacion - minTiempoFabricacion + 1) + minTiempoFabricacion;
		sleep(waitFabricacion);
		
		fabricacionTanda = rand() % (maxVacunasPorTanda - minVacunasPorTanda + 1) + minVacunasPorTanda;

		vacunasTotales[num-1] = vacunasTotales[num-1] + fabricacionTanda;
		if (vacunasTotales[num-1] > 400)
			fabricacionTanda = fabricacionTanda - (vacunasTotales[num-1] - 400);

		printf("Fábrica %d prepara %d vacunas\n", num, fabricacionTanda);
		fprintf(salida, "Fábrica %d prepara %d vacunas\n", num, fabricacionTanda);

		waitReparto = rand() % maxTiempoReparto + 1;
		sleep(waitReparto);

		pthread_mutex_lock(&mutex_fabricas);

		for (int i = 0; i < 5; i++)
			demandaTotal = demandaTotal + demanda[i];
		for (int i = 0; i < 5; i++) {
			vacunasFabricadas = fabricacionTanda * demanda[i] / demandaTotal;
			vacunas[i] = vacunas[i] + (int)vacunasFabricadas;
			printf("Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas, i+1);
			fprintf(salida, "Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas, i+1);
		}

		pthread_cond_signal(&disponible);

		pthread_mutex_unlock(&mutex_fabricas);

	}

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
