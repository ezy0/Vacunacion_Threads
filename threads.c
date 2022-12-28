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
// Cantidad de vacunados en una tanda
int	vacunadosTanda;

// Todos estos enteros nos serviran a modo de boolean
int fabricando = 0; // Para controlar cúando una fábrica está haciendo o no vacunas
int fin = 0;	//Para saber cúando han terminado las tandas, obligando a los threads de las fábricas a acabar aunque no hayan fabricado las 400 vacunas

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
void	configuracionInicial(FILE *salida);
void	estadisticasFinales(FILE *salida);

int	main(int argc, char *argv[]) {
	// Creación de threads para vacunar a cada habitante
	pthread_t	*threads;
	pthread_t	threadsF[3];
	int			idHabitante = 0;
	int			idHabitanteJoin = 0;
	int			aux;

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

	configuracionInicial(salida);

	//Repartir vacunas iniciales a cada centro
	for (int i = 0; i < 5; i++)
		vacunas[i] = numVacunasIniciales;

	// Inicialización de threads para cada fabrica
	for (int i = 1; i <= 3; i++) {
		aux = i;
		pthread_create(&threadsF[i], NULL, repartirFabrica, &i);
		while (aux == i)
			sleep(0.1);
		i = aux;
	}

	for (int j = 1; j <= 10; j++){
		printf("TANDA %d\n", j);
		vacunadosTanda = 0;
		// Inicialización de threads para vacunar a cada habitante
		for (int i = 1; i <= numHabitantes / 10; i++) {
			idHabitante++;
			aux = idHabitante;
			pthread_create(&threads[idHabitante], NULL, vacunarHabitante, &idHabitante);
			while (aux == idHabitante)
				sleep(0.1);
			idHabitante = aux;
		}

		// Espera a que todos los threads hayan terminado
		for (int i = 1; i <= numHabitantes / 10; i++) {
			idHabitanteJoin++;
			pthread_join(threads[idHabitanteJoin], NULL);
		}
	}

	fin = 1;

	// Espera a que todos los threads hayan terminado
	for (int i = 1; i <= 3; i++)
		pthread_join(threadsF[i], NULL);

	estadisticasFinales(salida);

	pthread_mutex_destroy(&mutex_vacunas);
	pthread_mutex_destroy(&mutex_fabricas);
	pthread_cond_destroy(&disponible);
	pthread_cond_destroy(&no_disponible);
	free(threads);
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
	fprintf(salida, "Habitante %d elige el centro %d para vacunarse\n", num, centroAsignado);

	// Una vez asignado, se espera a que se desplace al centro. En ese momento se hace el lock del mutex
	waitDesplazamiento = rand() % maxTiempoDesplazamiento + 1;
	sleep(waitDesplazamiento);

	pthread_mutex_lock(&mutex_vacunas);

	demanda[centroAsignado - 1]++;	//Aumentamos la demanda

	while (vacunas[centroAsignado - 1] == 0){//Comprobamos si quedan o no vacunas y esperamos en el caso de que no
		pthread_cond_signal(&no_disponible);
		pthread_cond_wait(&disponible, &mutex_vacunas);
	}
	pthread_mutex_unlock(&mutex_vacunas);
	pthread_mutex_lock(&mutex_vacunas);
	vacunas[centroAsignado - 1]--;	// Quitamos una vacuna
	demanda[centroAsignado - 1]--;	//Bajamos la demanda
	habitantesVacunadosTotales[centroAsignado - 1]++;
	vacunadosTanda++;
	printf("Habitante %d vacunado en el centro %d\n", num, centroAsignado);
	fprintf(salida, "Habitante %d vacunado en el centro %d\n", num, centroAsignado);

	//Mandamos una señal de que no quedan vacunas, en el caso de que el habitante haya sido la última.
	/*if (vacunas[centroAsignado - 1] == 0)
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

	while (vacunasTotales[num-1] <= 400 || fin == 1) {
		vacunasFabricadas = 0;
		demandaTotal = 0;
		fabricando = 0;

		pthread_mutex_lock(&mutex_fabricas);
		while (fabricando == 0) {
			pthread_cond_wait(&no_disponible, &mutex_fabricas);
			fabricando = 1;
		}
		pthread_mutex_unlock(&mutex_fabricas);
		fabricando = 0;

		waitFabricacion = rand() % (maxTiempoFabricacion - minTiempoFabricacion + 1) + minTiempoFabricacion;
		sleep(waitFabricacion);
		waitReparto = rand() % maxTiempoReparto + 1;	//Se ponen juntos para que no se cuele ningun otro thread entre medias
		sleep(waitReparto);
		
		fabricacionTanda = rand() % (maxVacunasPorTanda - minVacunasPorTanda + 1) + minVacunasPorTanda;

		vacunasTotales[num-1] = vacunasTotales[num-1] + fabricacionTanda;
		if (vacunasTotales[num-1] > 400)
			fabricacionTanda = fabricacionTanda - (vacunasTotales[num-1] - 400);

		printf("FABRICA %d ESPERANDO A NO DISPONIBLE\n", num);
		printf ("VACUNAS 1: %d, VACUNAS 2: %d, VACUNAS 3: %d, VACUNAS 4: %d, VACUNAS 5: %d\n", vacunas[0], vacunas[1], vacunas[2], vacunas[3], vacunas[4]);
		printf ("DEMANDA 1: %d, DEMANDA 2: %d, DEMANDA 3: %d, DEMANDA 4: %d, DEMANDA 5: %d\n", demanda[0], demanda[1], demanda[2], demanda[3], demanda[4]);

		pthread_mutex_lock(&mutex_fabricas);
	
		printf("Fábrica %d prepara %d vacunas\n", num, fabricacionTanda);
		fprintf(salida, "Fábrica %d prepara %d vacunas\n", num, fabricacionTanda);

		for (int i = 0; i < 5; i++)
			demandaTotal = demandaTotal + demanda[i];
		for (int i = 0; i < 5; i++) {
			printf("demanda %d / demanda total %d\n", demanda[i], demandaTotal);
			vacunasFabricadas = round(fabricacionTanda * demanda[i] / demandaTotal);
			vacunas[i] = vacunas[i] + vacunasFabricadas;
			vacunasRecibidasTotales[i] = vacunasRecibidasTotales[i] + vacunasFabricadas;
			vacunasEntregadasCentro[num-1][i] = vacunasEntregadasCentro[num-1][i] + vacunasFabricadas;
			printf("Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas, i+1);
			fprintf(salida, "Fábrica %d entrega %d vacunas en el centro %d\n", num, vacunasFabricadas, i+1);
		}
		
		for (int i = 0; i < demandaTotal; i++)
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

void	configuracionInicial(FILE *salida){

	printf("Habitantes: %d\n", numHabitantes);
	printf("Centros de vacunación: 5\n");
	printf("Fábricas: 3\n");
	printf("Vacunados por tanda: %d\n", numHabitantes / 10);
	printf("Vacunas iniciales en cada centro: %d\n", numVacunasIniciales);
	printf("Vacunas totales por fábrica: 400\n");
	printf("Mínimo número de vacunas fabricadas en cada tanda: %d\n", minVacunasPorTanda);
	printf("Máximo número de vacunas fabricadas en cada tanda: %d\n", maxVacunasPorTanda);
	printf("Tiempo máximo de reparto de vacunas a los centros: %d\n", maxTiempoReparto);
	printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %d\n", maxTiempoReaccion);
	printf("Tiempo máximo de desplazamineto del habitante al centro de vacunación: %d\n", maxTiempoDesplazamiento);

	fprintf(salida, "Habitantes: %d\n", numHabitantes);
	fprintf(salida, "Centros de vacunación: 5\n");
	fprintf(salida, "Fábricas: 3\n");
	fprintf(salida, "Vacunados por tanda: %d\n", numHabitantes / 10);
	fprintf(salida, "Vacunas iniciales en cada centro: %d\n", numVacunasIniciales);
	fprintf(salida, "Vacunas totales por fábrica: 400\n");
	fprintf(salida, "Mínimo número de vacunas fabricadas en cada tanda: %d\n", minVacunasPorTanda);
	fprintf(salida, "Máximo número de vacunas fabricadas en cada tanda: %d\n", maxVacunasPorTanda);
	fprintf(salida, "Tiempo máximo de reparto de vacunas a los centros: %d\n", maxTiempoReparto);
	fprintf(salida, "Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %d\n", maxTiempoReaccion);
	fprintf(salida, "Tiempo máximo de desplazamineto del habitante al centro de vacunación: %d\n", maxTiempoDesplazamiento);
}

void	estadisticasFinales(FILE *salida){

	for (int i = 0; i < 3; i++){
		printf("La fábrica %d ha fabricado %d vacunas totales\n", i + 1, vacunasTotales[i]);
		fprintf(salida, "La fábrica %d ha fabricado %d vacunas totales\n", i + 1, vacunasTotales[i]);
		for (int j = 0; j < 5; j++){
			printf("La fábrica %d ha entregado %d vacunas totales al centro %d\n", i+1, vacunasEntregadasCentro[i][j], j+1);
			fprintf(salida, "La fábrica %d ha entregado %d vacunas totales al centro %d\n", i+1, vacunasEntregadasCentro[i][j], j+1);
		}
	}

	for (int i = 0; i < 5; i++){
		printf("El centro %d ha recibido un total de %d vacunas\n", i + 1, vacunasRecibidasTotales[i]);
		fprintf(salida, "El centro %d ha recibido un total de %d vacunas\n", i + 1, vacunasRecibidasTotales[i]);
		printf("En el centro %d se han vacunado un total de %d habitantes\n", i + 1, habitantesVacunadosTotales[i]);
		fprintf("En el centro %d se han vacunado un total de %d habitantes\n", i + 1, habitantesVacunadosTotales[i]);
		printf("En el centro %d han acabado sobrando %d vacunas\n", i + 1, vacunas[i]);
		fprintf(salida, "En el centro %d han acabado sobrando %d vacunas\n", i + 1, vacunas[i]);
	}

}