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

//Estos tres arrays de enteros son para guardar los datos necesarios que nos permitirán hacer las estadísticas finales
int vacunasEntregadasCentro[3][5];
int	vacunasRecibidasTotales[5];
int habitantesVacunadosTotales[5];


// Creacion del mutex de las vacunas por centro y de las condiciones
pthread_mutex_t mutex;
pthread_cond_t disponible;

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
	int			i, j;
	int			*idHab, idFab[3];

	for (i = 0; i < 5; i++)
		demanda[i] = 0;

	for (i = 0; i < 3; i++)
		vacunasTotales[i] = 0;

	// Inicializacion del mutex y de las condiciones
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&disponible, NULL);


	// Llamamos a leer fichero para inicializar el resto de variables y declaramos el fichero de salida
	if (argc < 2) {
		leerFichero("entrada_vacunacion.txt");
		salida = fopen("salida_vacunacion.txt", "w");
	}
	else if (argc == 2) {
		if (access(argv[1], R_OK) != 0){
			printf ("El archivo introducido de lectura no existe\n");
			return (1);
		}
		leerFichero(argv[1]);
		salida = fopen("salida_vacunacion.txt", "w");
	}
	else {
		if (access(argv[1], R_OK) != 0){
			printf ("El archivo introducido de lectura no existe\n");
			return (1);
		}
		leerFichero(argv[1]);
		salida = fopen(argv[2], "w");
	}
	if (salida == NULL)
	{
		printf("No se ha podido abrir el archivo para esciribir, se cerrará el programa\n");
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&disponible);
		return 1;
	}

	configuracionInicial();

	//Asignamos espacio para los threads y las ID de los habitantes con malloc
	threads = (pthread_t *)malloc(sizeof(pthread_t) * numHabitantes);
	idHab = (int *)malloc(sizeof(int) * numHabitantes);

	//Repartir vacunas iniciales a cada centro
	for (i = 0; i < 5; i++)
		vacunas[i] = numVacunasIniciales;

	// Inicialización de threads para cada fabrica
	for (i = 1; i <= 3; i++) {
		idFab[i - 1] = i;
		pthread_create(&threadsF[i-1], NULL, repartirFabrica, (void *)&idFab[i-1]);
	}

	for (j = 1; j <= 10; j++){
		// Inicialización de threads para vacunar a cada habitante
		for (i = 1; i <= numHabitantes / 10; i++) {
			idHabitante++;
			idHab[idHabitante - 1] = idHabitante;
			pthread_create(&threads[idHabitante-1], NULL, vacunarHabitante, (void *)&idHab[idHabitante - 1]);
		}

		// Espera a que todos los threads hayan terminado
		for (i = 1; i <= numHabitantes / 10; i++) {
			idHabitanteJoin++;
			pthread_join(threads[idHabitanteJoin-1], NULL);
		}
	}


	// Espera a que todos los threads hayan terminado
	for (i = 1; i <= 3; i++)
		pthread_join(threadsF[i-1], NULL);

	printf ("\n¡TODOS LOS HABITANTES SE HAN VACUNADO!\n");
	fprintf (salida, "\n¡TODOS LOS HABITANTES SE HAN VACUNADO!\n");

	estadisticasFinales(salida);

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&disponible);

	fclose(salida);

	return 0;
}

// Función que ejecutará cada thread para vacunar a un habitante
void	*vacunarHabitante(void *arg) {
	int num = *(int *)arg;	//numero del habitante
	int	centroAsignado;
	int	waitReaccion, waitDesplazamiento;

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

	pthread_mutex_lock(&mutex);

	demanda[centroAsignado - 1]++;	//Aumentamos la demanda

	while (vacunas[centroAsignado - 1] == 0)//Comprobamos si quedan o no vacunas y esperamos en el caso de que no
		pthread_cond_wait(&disponible, &mutex);

	vacunas[centroAsignado - 1]--;	// Quitamos una vacuna
	demanda[centroAsignado - 1]--;	//Bajamos la demanda
	habitantesVacunadosTotales[centroAsignado - 1]++;
	
	/*if (vacunas[0] != 0 && vacunas[1] != 0 && vacunas[2] != 0 && vacunas[3] != 0 && vacunas[4] != 0)
		pthread_cond_signal(&disponible);*/

	pthread_mutex_unlock(&mutex);

	printf("Habitante %d vacunado en el centro %d\n", num, centroAsignado);
	fprintf(salida, "Habitante %d vacunado en el centro %d\n", num, centroAsignado);

	return 0;
}

void	*repartirFabrica(void *arg) {
	int num = *(int *)arg;	//numero de la fabrica
	int fabricacionTanda;
	int waitFabricacion, waitReparto;
	int demandaTotal;
	int	vacunasFabricadas[5];
	int	sumaVacunasFabricadas;
	int	vacunasAux;
	int	centroRand;
	int i;

	while (vacunasTotales[num-1] < numHabitantes/3) {
		for(i = 0; i < 5; i++)
			vacunasFabricadas[i] = 0;
		demandaTotal = 0;
		sumaVacunasFabricadas = 0;
		vacunasAux = 0;

		waitFabricacion = rand() % (maxTiempoFabricacion - minTiempoFabricacion + 1) + minTiempoFabricacion;
		sleep(waitFabricacion);
		
		//Fabricacion de vacunas
		fabricacionTanda = rand() % (maxVacunasPorTanda - minVacunasPorTanda + 1) + minVacunasPorTanda; //Calculas las vacunas que vas a hacer en esta tanda

		vacunasTotales[num-1] = vacunasTotales[num-1] + fabricacionTanda;	//Añade las vacunas fabricadas para saber si llega al tope
		// Comprobamos si nos pasamos del tope de vacunas que podemos fabricas, y si nos pasamos, modificamos las vacunas que hemos fabricado
		if (vacunasTotales[num-1] > numHabitantes/3){
			vacunasAux = vacunasTotales[num-1];
			vacunasTotales[num-1] = vacunasTotales[num-1] - fabricacionTanda;
			fabricacionTanda = fabricacionTanda - (vacunasAux - numHabitantes/3);
			vacunasTotales[num-1] = vacunasTotales[num-1] + fabricacionTanda;
		}

		printf("Fábrica %d ha preparado %d vacunas\n", num, fabricacionTanda);
		fprintf(salida, "Fábrica %d ha preparado %d vacunas\n", num, fabricacionTanda);

		waitReparto = rand() % maxTiempoReparto + 1;
		sleep(waitReparto);

		//REPARTO
		pthread_mutex_lock(&mutex);	

		// CALCULO DE LAS VACUNAS QUE SE VAN A REPARTIR
		for (i = 0; i < 5; i++)	//Calculo de la demanda total
			demandaTotal = demandaTotal + demanda[i];

		if (demandaTotal > 0 && demandaTotal <= fabricacionTanda) //Reparto segun demanda
			for (i = 0; i < 5; i++)
				vacunasFabricadas[i] = demanda[i];

		//Comprobacion por si faltan vacunas al repartir
		for (i = 0; i < 5; i++)
			sumaVacunasFabricadas = sumaVacunasFabricadas + vacunasFabricadas[i];

		if (sumaVacunasFabricadas < fabricacionTanda)	//Si no se han repartido todas se repartes equitativamente las que sobran
			for (i = 0; i < 5; i++)
				vacunasFabricadas[i] = vacunasFabricadas[i] + ((fabricacionTanda - sumaVacunasFabricadas) / 5);

		//Se vuelve a comprobar si faltan vacunas a repartir
		sumaVacunasFabricadas = 0;
		for (i = 0; i < 5; i++)
			sumaVacunasFabricadas = sumaVacunasFabricadas + vacunasFabricadas[i];
	
		centroRand = rand() % 5; //Centro aleatorio donde se repartiran las vacunas sobrantes
		if (sumaVacunasFabricadas < fabricacionTanda)
			vacunasFabricadas[centroRand] = vacunasFabricadas[centroRand] + (fabricacionTanda - sumaVacunasFabricadas);

		for (i = 0; i < 5; i++) {	//Actualizacion de la cantidad de vacunas y estadisticas
			vacunas[i] = vacunas[i] + vacunasFabricadas[i];
			//Estadisticas
			vacunasRecibidasTotales[i] = vacunasRecibidasTotales[i] + vacunasFabricadas[i];
			vacunasEntregadasCentro[num-1][i] = vacunasEntregadasCentro[num-1][i] + vacunasFabricadas[i];

			printf("Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas[i], i+1);
			fprintf(salida, "Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas[i], i+1);
		}	

		for (i = 0; i < demandaTotal; i++)	//Se mandan tantas señales como habitantes esperando haya
			pthread_cond_signal(&disponible);

		pthread_mutex_unlock(&mutex);
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
	fprintf(salida, "PROCESO DE VACUNACIÓN\n");

}

void	estadisticasFinales(FILE *salida){

	int	i, j;

	printf("\nESTADISTICAS FINALES: \n");
	fprintf(salida, "\nESTADISTICAS FINALES: \n");

	for (i = 0; i < 3; i++){
		printf("La fábrica %d ha fabricado %d vacunas totales\n", i + 1, vacunasTotales[i]);
		fprintf(salida, "La fábrica %d ha fabricado %d vacunas totales\n", i + 1, vacunasTotales[i]);
		for (j = 0; j < 5; j++){
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