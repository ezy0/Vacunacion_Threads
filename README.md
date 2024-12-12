# Simulación de Vacunación con Threads

## Descripción
Este proyecto implementa una simulación de vacunación masiva ante una pandemia en un país pequeño. Utilizando programación concurrente, se coordina el trabajo de:

- Tres empresas farmacéuticas encargadas de la fabricación y distribución de vacunas.
- Cinco centros de vacunación distribuidos por el territorio.
- Habitantes que acuden a vacunarse siguiendo un esquema organizado.

El sistema garantiza que toda la población sea vacunada en 10 tandas, asegurando la sincronización adecuada y evitando problemas como condiciones de carrera o inanición.

## Características principales
- **Lectura de configuración** desde un archivo de texto para establecer parámetros iniciales.
- **Gestor de threads** para modelar el comportamiento de las empresas farmacéuticas y los habitantes.
- **Control eficiente de recursos compartidos** mediante `mutex` y condiciones.
- **Generación de estadísticas detalladas** al finalizar el proceso de vacunación.
- **Salida dual:** impresión en pantalla y almacenamiento en un archivo de texto.

## Principales funciones
- **main:**
  - Inicializa mutex y condiciones.
  - Crea threads para modelar las empresas y los habitantes.
  - Implementa las tandas de vacunación, asegurándose de que todas finalicen correctamente.

- **vacunarHabitante(void *arg):**
  - Simula el proceso de vacunación de un habitante, desde su selección del centro hasta la vacunación.
  - Gestiona tiempos de reacción y desplazamiento generados aleatoriamente.

- **repartirFabrica(void *arg):**
  - Simula la fabricación y distribución de vacunas a los centros.
  - Controla la asignación de vacunas en función de la demanda existente.
  - Emite señales para notificar a los habitantes esperando en los centros.

### Archivo de Configuración
El archivo de entrada define los parámetros del sistema mediante un entero por línea:
1. Habitantes totales.
2. Vacunas iniciales por centro.
3. Mínimo de vacunas producidas por tanda.
4. Máximo de vacunas producidas por tanda.
5. Tiempo mínimo de fabricación de una tanda.
6. Tiempo máximo de fabricación de una tanda.
7. Tiempo máximo de distribución de una tanda.
8. Tiempo máximo de reacción de un habitante.
9. Tiempo máximo de desplazamiento al centro.

## Ejecución
El programa utiliza un archivo de configuración para inicializar los valores del sistema, como el número de habitantes, las vacunas iniciales y los tiempos de fabricación y distribución.

## Compilación

Para compilar el programa, utiliza el siguiente comando:

```bash
gcc -Wall -Wextra -Werror vacunacion.c -lpthread -o vacunacion
```

## Autores
- https://github.com/ezy0
- https://github.com/a-martinma
