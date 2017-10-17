#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

#define BUFSIZE 255

typedef sem_t semaforo;
//Encabezado que se recibe del cliente con la iformación del archivo
typedef struct {
	char identificacionEstudiantes[BUFSIZE];
	char nombreArchivo[BUFSIZE];
	size_t tamanio;
}header;

int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes);
void obtenerNuevoNombre(char * nombreArchivo);
//Funcion que realiza la comunicacion con el cliente
void * transferencia(void *);

semaforo mutex;

int main(int argc, char * argv[]) {
	int _socket;
	int conexion;
	struct sockaddr_in addr; /* Direccion de IPv4 */
	struct sockaddr_in c_addr; /* Direccion de IPv4 */
	sem_init(&mutex, 0, 1); 

	socklen_t c_addr_len;
	int val, descriptorArchivo;
	int finalizacion;
	pthread_t hilo_transferencia;

  //Crear el socket
	_socket = socket(PF_INET, SOCK_STREAM, 0);

  //Configurar el socket para reutilizar la direccion
	val = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

//Rellenar con ceros
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2510);
	addr.sin_addr.s_addr = INADDR_ANY; //En todas las direcciones

//Asociar la direccion INDARR_ANY y el puerto 2510 al socket
	if (bind(_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

  //Manifestar el deseo de recibir conexiones
  	if (listen(_socket, 100) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

  //Variable que controla el servidor
	finalizacion = 0;

	while (!finalizacion) {
    //Bloquearse hasta que se reciba una conexion
		printf("Esperando conexion\n");
		c_addr_len = sizeof(struct sockaddr_in);
		conexion = accept(_socket, (struct sockaddr *)&c_addr, &c_addr_len);
		if (conexion == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		pthread_create(&hilo_transferencia, 0, transferencia, (void *)conexion);
	}

	close(_socket); //Cerrar el socket
	fprintf(stderr, "Server finished\n");
	exit(EXIT_SUCCESS);
}

//Funcion que realiza la comunicacion con el cliente
void * transferencia(void * conexion) {
	header encabezado;
	int finalizacion = 0;
	char buf[BUFSIZ];
	int n, descriptorArchivo, bytesEscritos;
  //Comunicacion con el cliente
  //1. Obtener la informacion del archivo a recibir
	if (read(conexion, &encabezado, sizeof(header)) != sizeof(header)) {
		fprintf(stderr, "Error al recibir el encabezado\n");
		close(conexion);
		return;
	}
	descriptorArchivo = crearArchivo(encabezado.nombreArchivo, encabezado.identificacionEstudiantes);

	printf("descriptor %d", descriptorArchivo);
  //enviar a la salida estandar la informacion del cliente
	fprintf(stdout, "\nRecibiendo archivo %s - Enviado por %s - Tamanio %d\n\n", encabezado.nombreArchivo, encabezado.identificacionEstudiantes, encabezado.tamanio);
  //2. Leer los datos del archivo enviados por el socket
	while(!finalizacion) {
		memset(&buf, 0, BUFSIZ);
		n = read(conexion, buf, BUFSIZ); //Leer del socket
		if (n <= 0) {
			finalizacion = 1;
		} else {
			bytesEscritos = write(descriptorArchivo, &buf, BUFSIZ);
		}
	}
	close(descriptorArchivo);
}

int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes) {
	int longitudRuta, descriptorArchivo = 0, tamanioPrimerBuffer, tamanioSegundoBuffer;
	time_t tiempo = time(0);
	struct tm * local = localtime(&tiempo);
	char bufferFecha[20], * rutaAlmacenamiento, * segundoBuffer, * primerBuffer, * copiaNombre;

	strftime(bufferFecha, 20,"%d_%m_%Y_%H_%M_%S", local);

	tamanioPrimerBuffer = strlen(nombreArchivo) + 12;
	tamanioSegundoBuffer = strlen(bufferFecha) + strlen(identificacionEstudiantes) + 1;
	primerBuffer = (char *)malloc(tamanioPrimerBuffer);

	memset(primerBuffer, 0, tamanioPrimerBuffer);
	sprintf(primerBuffer, "recibidos/%s", nombreArchivo);

	segundoBuffer = (char *)malloc(tamanioSegundoBuffer);
	memset(segundoBuffer, 0, tamanioSegundoBuffer);
	sprintf(segundoBuffer, "_%s_%s", identificacionEstudiantes, bufferFecha);

	printf("tamanio segundo Buffer [%d] [%s]\n\n", tamanioSegundoBuffer, segundoBuffer);
	longitudRuta = tamanioPrimerBuffer + tamanioSegundoBuffer;
	rutaAlmacenamiento = (char *)malloc(longitudRuta);
	memset(rutaAlmacenamiento, 0, longitudRuta);
	sprintf(rutaAlmacenamiento, "%s%s", primerBuffer, segundoBuffer);

	printf("Ruta de almacenamiento %s\n", rutaAlmacenamiento);

	sem_wait(&mutex);
	descriptorArchivo = open(rutaAlmacenamiento, O_CREAT | O_EXCL | O_WRONLY, 00660);
	sem_post(&mutex);
	while (descriptorArchivo == -1) {
		copiaNombre = (char *)malloc(strlen(nombreArchivo));
	        memset(copiaNombre, 0, strlen(nombreArchivo));	
		sprintf(copiaNombre, "%s", nombreArchivo);
		obtenerNuevoNombre(primerBuffer);
		rutaAlmacenamiento = (char *)malloc(strlen(copiaNombre) + strlen(segundoBuffer));
		memset(rutaAlmacenamiento, 0,strlen(primerBuffer) + strlen(segundoBuffer));
		sprintf(rutaAlmacenamiento, "%s%s", primerBuffer, segundoBuffer);
		printf("Nueva Ruta de Almacenamiento %s\n", rutaAlmacenamiento);
		sem_wait(&mutex);
		descriptorArchivo = open(rutaAlmacenamiento, O_CREAT | O_WRONLY, 00660);
		sem_post(&mutex);
	}
	return descriptorArchivo;
}

void obtenerNuevoNombre(char * nombreArchivo) {
	char * nombre, * extension, cadenaNumero[10];
	int indice = 0, numeroAleatorio = 0, longitud = 0;
	memset(&cadenaNumero, 0, 10);
	numeroAleatorio = rand() % 999999999;
	sprintf(cadenaNumero, "%d", numeroAleatorio);
	extension = strchr(nombreArchivo, '.');
	longitud = strlen(cadenaNumero);
	longitud++;
	if (extension != NULL) {
		indice = extension - nombreArchivo + 1;
		strncpy(nombre, nombreArchivo, indice);		
		longitud += strlen(nombre) + strlen(extension);
		nombreArchivo = (char *)malloc(longitud);
		memset(nombreArchivo, 0, longitud);
		sprintf(nombreArchivo, "%s_%s%s", nombre, cadenaNumero, extension);
	} else {
		longitud += strlen(nombreArchivo);
		nombre = (char *)malloc(strlen(nombreArchivo));
		memset(nombre, 0, strlen(nombreArchivo));
		sprintf(nombre, "%s", nombreArchivo);
		nombreArchivo = (char *)malloc(longitud);
		memset(nombreArchivo, 0, longitud);
		sprintf(nombreArchivo, "%s_%s%s", nombre, cadenaNumero, extension);
	}
}

