#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
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

int fabricaFin[3];	//Para saber cuando ha acabado cada fabrica
int creandoVacunas = 0;	//Para evitar que los habitantes manden la señal mientras haya alguna fabrica trabajando.

// Todos estos enteros nos serviran a modo de boolean
int fabricando = 0; // Para controlar cúando una fábrica está haciendo o no vacunas
int fin = 0;	//Para saber cúando han terminado las tandas, obligando a los threads de las fábricas a acabar aunque no hayan fabricado el tope

//Estos tres arrays de enteros son para guardar los datos necesarios que nos permitirán hacer las estadísticas finales
int vacunasEntregadasCentro[3][5];
int	vacunasRecibidasTotales[5];
int habitantesVacunadosTotales[5];


// Creacion del mutex de las vacunas por centro y de las condiciones
pthread_mutex_t mutex_vacunas, mutex_fabricas;
pthread_cond_t disponible;
pthread_cond_t no_disponible;

// Declaración del fichero por donde saldrá el resultado final
FILE	*salida;

// Declaración de funciones
void	*vacunarHabitante(void *arg); // Función que ejecutará cada thread para vacunar a un habitante
void	leerFichero(char *fichero); //Damos valores a las variables que faltan por inicializar
void	*repartirFabrica(void *arg);
void	configuracionInicial();
void	estadisticasFinales(FILE *salida);

int	main(int argc, char *argv[]) {
	// Creación de threads para vacunar a cada habitante
	pthread_t	*threads;
	pthread_t	threadsF[3];
	//Creacion de variables locales
	int			idHabitante = 0;
	int			idHabitanteJoin = 0;
	int			aux;

	//Asignamos espacio para los threads de los habitantes con malloc
	threads = (pthread_t *)malloc(sizeof(pthread_t) * numHabitantes);

	for (int i = 0; i < 5; i++)
		demanda[i] = 0;

	for (int i = 0; i < 3; i++){
		vacunasTotales[i] = 0;
		fabricaFin[i] = 0;
	}

	// Inicializacion del mutex y de las condiciones
	pthread_mutex_init(&mutex_vacunas, NULL);
	pthread_mutex_init(&mutex_fabricas, NULL);
	pthread_cond_init(&disponible, NULL);
	pthread_cond_init(&no_disponible, NULL);

	// Llamamos a leer fichero para inicializar el resto de variables y declaramos el fichero de salida
	if (argc < 2) {
		leerFichero("entrada_vacunacion.txt");
		salida = fopen("salida_vacunacion.txt", "a");
	}
	else if (argc == 2) {
		if (access(argv[1], R_OK) != 0){
			printf ("El archivo introducido de lectura no existe\n");
			return (1);
		}
		leerFichero(argv[1]);
		salida = fopen("salida_vacunacion.txt", "a");
	}
	else {
		if (access(argv[1], R_OK) != 0){
			printf ("El archivo introducido de lectura no existe\n");
			return (1);
		}
		leerFichero(argv[1]);
		salida = fopen(argv[2], "a");
	}
	if (salida == NULL)
	{
		printf("No se ha podido abrir el archivo para esciribir, se cerrará el programa\n");
		pthread_mutex_destroy(&mutex_vacunas);
		pthread_mutex_destroy(&mutex_fabricas);
		pthread_cond_destroy(&disponible);
		pthread_cond_destroy(&no_disponible);
		free(threads);
		return 1;
	}

	configuracionInicial();

	//Repartir vacunas iniciales a cada centro
	for (int i = 0; i < 5; i++)
		vacunas[i] = numVacunasIniciales;

	// Inicialización de threads para cada fabrica
	for (int i = 1; i <= 3; i++) {
		aux = i;
		pthread_create(&threadsF[i-1], NULL, repartirFabrica, &i);
		while (aux == i)
			sleep(0.1);
		i = aux;
	}

	for (int j = 1; j <= 10; j++){
		// Inicialización de threads para vacunar a cada habitante
		for (int i = 1; i <= numHabitantes / 10; i++) {
			idHabitante++;
			aux = idHabitante;
			pthread_create(&threads[idHabitante-1], NULL, vacunarHabitante, &idHabitante);
			while (aux == idHabitante)
				sleep(0.1);
			idHabitante = aux;
		}

		// Espera a que todos los threads hayan terminado
		for (int i = 1; i <= numHabitantes / 10; i++) {
			idHabitanteJoin++;
			pthread_join(threads[idHabitanteJoin-1], NULL);
		}
	}

	fin = 1;

	// Espera a que todos los threads hayan terminado
	for (int i = 1; i <= 3; i++){
		while(fabricaFin[0] == 0 || fabricaFin[1] == 0 || fabricaFin[2] == 0)
			pthread_cond_signal(&no_disponible);
		pthread_join(threadsF[i-1], NULL);
	}

	estadisticasFinales(salida);

	pthread_mutex_destroy(&mutex_vacunas);
	pthread_mutex_destroy(&mutex_fabricas);
	pthread_cond_destroy(&disponible);
	pthread_cond_destroy(&no_disponible);

	return 0;
}

// Función que ejecutará cada thread para vacunar a un habitante
void	*vacunarHabitante(void *arg) {
	int num = *(int *)arg;	//numero del habitante
	*(int *)arg = 0;	// Sirve de indicador para saber que el numero ya se ha almacenado y no se va a repetir
	int	centroAsignado;
	int	waitReaccion, waitDesplazamiento;

	// Esperar a que se entere de la vacunacion
	waitReaccion = rand() % maxTiempoReaccion + 1;
	sleep(waitReaccion);
	
	//Una vez se entera se le asigna el centro con random y escribimos
	centroAsignado = rand() % 5 + 1;

	printf("Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);
	//fprintf(salida, "Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);
	
	// Una vez asignado, se espera a que se desplace al centro. En ese momento se hace el lock del mutex
	waitDesplazamiento = rand() % maxTiempoDesplazamiento + 1;
	sleep(waitDesplazamiento);

	pthread_mutex_lock(&mutex_vacunas);

	demanda[centroAsignado - 1]++;	//Aumentamos la demanda

	while (vacunas[centroAsignado - 1] == 0){//Comprobamos si quedan o no vacunas y esperamos en el caso de que no
		if (creandoVacunas == 0)
			pthread_cond_signal(&no_disponible);
		pthread_cond_wait(&disponible, &mutex_vacunas);
	}

	vacunas[centroAsignado - 1]--;	// Quitamos una vacuna
	demanda[centroAsignado - 1]--;	//Bajamos la demanda
	habitantesVacunadosTotales[centroAsignado - 1]++;
	printf("Habitante %d vacunado en el centro %d\n", num, centroAsignado);
	//fprintf(salida, "Habitante %d vacunado en el centro %d\n", num, centroAsignado);
	
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

	while (vacunasTotales[num-1] <= numHabitantes/3 && fin == 0) {
		vacunasFabricadas = 0;
		demandaTotal = 0;
		fabricando = 0;
		creandoVacunas = 0;

		pthread_mutex_lock(&mutex_fabricas);
		while (fabricando == 0 && fin == 0) {
			pthread_cond_wait(&no_disponible, &mutex_fabricas);
			creandoVacunas = 1;
			fabricando = 1;
		}
		if (fin == 1){
			pthread_mutex_unlock(&mutex_fabricas);
			fabricaFin[num-1] = 1;
			return 0;
		}

		pthread_mutex_unlock(&mutex_fabricas);
		fabricando = 0;

		waitFabricacion = rand() % (maxTiempoFabricacion - minTiempoFabricacion + 1) + minTiempoFabricacion;
		sleep(waitFabricacion);
		waitReparto = rand() % maxTiempoReparto + 1;	//Se ponen juntos para que no se cuele ningun otro thread entre medias
		sleep(waitReparto);
		
		fabricacionTanda = rand() % (maxVacunasPorTanda - minVacunasPorTanda + 1) + minVacunasPorTanda; //Calculas las vacunas que vas a hacer en esta tanda

		vacunasTotales[num-1] = vacunasTotales[num-1] + fabricacionTanda;	//Añade las vacunas fabricadas para saber si llega al tope
		if (vacunasTotales[num-1] > numHabitantes/3)
			fabricacionTanda = fabricacionTanda - (vacunasTotales[num-1] - numHabitantes/3);

		pthread_mutex_lock(&mutex_fabricas);

		printf("Fábrica %d prepara %d vacunas\n", num, fabricacionTanda);
		//fprintf(salida, "Fábrica %d prepara %d vacunas\n", num, fabricacionTanda);

		for (int i = 0; i < 5; i++)
			demandaTotal = demandaTotal + demanda[i];
		for (int i = 0; i < 5; i++) {
			if (demandaTotal == 0)
				demandaTotal = 1;

			vacunasFabricadas = round(fabricacionTanda * demanda[i] / demandaTotal); //Calcula las vacunas que tiene que fabricar
			vacunas[i] = vacunas[i] + vacunasFabricadas; // Se suman las vacunas a las que se han fabricado
			//Aqui recogemos todas las vacunas recibidas y las entregadas a cada centro, es para las estadisticas finales
			vacunasRecibidasTotales[i] = vacunasRecibidasTotales[i] + vacunasFabricadas;
			vacunasEntregadasCentro[num-1][i] = vacunasEntregadasCentro[num-1][i] + vacunasFabricadas;

			printf("Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas, i+1);
			//fprintf(salida, "Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas, i+1);
		}
		
		for (int i = 0; i < demandaTotal; i++)	//Mandamos tantas señales como habitantes las estén esperando
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

void	configuracionInicial(){

	printf("VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
	printf("Habitantes: %d\n", numHabitantes);
	printf("Centros de vacunación: 5\n");
	printf("Fábricas: 3\n");
	printf("Vacunados por tanda: %d\n", numHabitantes / 10);
	printf("Vacunas iniciales en cada centro: %d\n", numVacunasIniciales);
	printf("Vacunas totales por fábrica: %d\n", numHabitantes/3);
	printf("Mínimo número de vacunas fabricadas en cada tanda: %d\n", minVacunasPorTanda);
	printf("Máximo número de vacunas fabricadas en cada tanda: %d\n", maxVacunasPorTanda);
	printf("Tiempo máximo de reparto de vacunas a los centros: %d\n", maxTiempoReparto);
	printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %d\n", maxTiempoReaccion);
	printf("Tiempo máximo de desplazamineto del habitante al centro de vacunación: %d\n", maxTiempoDesplazamiento);
	printf("\n");
	printf("PROCESO DE VACUNACIÓN\n");
}

void	estadisticasFinales(FILE *salida){
	fprintf(salida, "VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
	fprintf(salida, "Habitantes: %d\n", numHabitantes);
	fprintf(salida, "Centros de vacunación: 5\n");
	fprintf(salida, "Fábricas: 3\n");
	fprintf(salida, "Vacunados por tanda: %d\n", numHabitantes / 10);
	fprintf(salida, "Vacunas iniciales en cada centro: %d\n", numVacunasIniciales);
	fprintf(salida, "Vacunas totales por fábrica: %d\n", numHabitantes/3);
	fprintf(salida, "Mínimo número de vacunas fabricadas en cada tanda: %d\n", minVacunasPorTanda);
	fprintf(salida, "Máximo número de vacunas fabricadas en cada tanda: %d\n", maxVacunasPorTanda);
	fprintf(salida, "Tiempo máximo de reparto de vacunas a los centros: %d\n", maxTiempoReparto);
	fprintf(salida, "Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %d\n", maxTiempoReaccion);
	fprintf(salida, "Tiempo máximo de desplazamineto del habitante al centro de vacunación: %d\n", maxTiempoDesplazamiento);
	fprintf(salida, "\n");

	for (int i = 0; i < 3; i++){
		printf("La fábrica %d ha fabricado %d vacunas totales\n", i + 1, vacunasTotales[i]);
		fprintf(salida, "La fábrica %d ha fabricado %d vacunas totales\n", i + 1, vacunasTotales[i]);
		for (int j = 0; j < 5; j++){
			printf("La fábrica %d ha entregado %d vacunas totales al centro %d\n", i+1, vacunasEntregadasCentro[i][j], j+1);
			fprintf(salida, "La fábrica %d ha entregado %d vacunas totales al centro %d\n", i+1, vacunasEntregadasCentro[i][j], j+1);
		}
		printf("\n");
		fprintf(salida, "\n");
	}

	for (int i = 0; i < 5; i++){
		printf("El centro %d ha recibido un total de %d vacunas\n", i + 1, vacunasRecibidasTotales[i]);
		fprintf(salida, "El centro %d ha recibido un total de %d vacunas\n", i + 1, vacunasRecibidasTotales[i]);
		printf("En el centro %d se han vacunado un total de %d habitantes\n", i + 1, habitantesVacunadosTotales[i]);
		fprintf(salida, "En el centro %d se han vacunado un total de %d habitantes\n", i + 1, habitantesVacunadosTotales[i]);
		printf("En el centro %d han acabado sobrando %d vacunas\n", i + 1, vacunas[i]);
		fprintf(salida, "En el centro %d han acabado sobrando %d vacunas\n", i + 1, vacunas[i]);
		printf("\n");
		fprintf(salida, "\n");
	}
}
