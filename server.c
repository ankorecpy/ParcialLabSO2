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

#define BUFSIZE 255

//Encabezado que se recibe del cliente con la iformación del archivo
typedef struct {
	char identificacionEstudiantes[BUFSIZE];
	char nombreArchivo[BUFSIZE];
	size_t tamanio;
}header;

int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes);
//Funcion que realiza la comunicacion con el cliente
void * transferencia(void *);

int main(int argc, char * argv[]) {
	int _socket;
	int conexion;
	struct sockaddr_in addr; /* Direccion de IPv4 */
	struct sockaddr_in c_addr; /* Direccion de IPv4 */

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
	int n, descriptorArchivo, longitudRuta, bytesEscritos;
  //Comunicacion con el cliente
  //1. Obtener la informacion del archivo a recibir
	if (read(conexion, &encabezado, sizeof(header)) != sizeof(header)) {
		fprintf(stderr, "Error al recibir el encabezado\n");
		close(conexion);
		return;
	}
	descriptorArchivo = crearArchivo(encabezado.nombreArchivo, encabezado.identificacionEstudiantes);

  //enviar a la salida estandar la informacion del cliente
	fprintf(stdout, "\nRecibiendo archivo %s - Enviado por %s - Tamanio %d\n\n", encabezado.nombreArchivo, encabezado.identificacionEstudiantes, encabezado.tamanio);
  //2. Leer los datos del archivo enviados por el socket
	while(!finalizacion) {
		memset(&buf, 0, BUFSIZ);
		n = read(conexion, buf, BUFSIZ); //Leer del socket
		if (n <= 0) {
			finalizacion = 1;
		} else {
			bytesEscritos = write(descriptorArchivo, &buf, BUFSIZ); //Enviar a la salida estandar
		}
	}
	close(descriptorArchivo);
}

int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes) {
	int longitudRuta, descriptorArchivo, numeroAleatorio, tamanioPrimerBuffer, tamanioSegundoBuffer;
	time_t tiempo = time(0);
	struct tm * local = localtime(&tiempo);
	char bufferFecha[20], * rutaAlmacenamiento, cadenaNumero[5] = "\0", * segundoBuffer, * primerBuffer;

	strftime(bufferFecha, 20,"%d_%m_%Y_%H_%M_%S", local);
	tamanioPrimerBuffer = strlen(nombreArchivo) + 12;
	tamanioSegundoBuffer = strlen(bufferFecha) + strlen(identificacionEstudiantes) + 1;
	primerBuffer = (char *)malloc(tamanioPrimerBuffer);
	segundoBuffer = (char *)malloc(tamanioSegundoBuffer);
	strcpy(segundoBuffer, "_");
	strcat(segundoBuffer, identificacionEstudiantes);
	strcat(segundoBuffer, "_");
	strcat(segundoBuffer, bufferFecha);
	strcpy(primerBuffer, "recibidos/");
	strcat(primerBuffer, nombreArchivo);
	longitudRuta = tamanioPrimerBuffer + tamanioSegundoBuffer;
	rutaAlmacenamiento = (char *)malloc(longitudRuta);
	memset(rutaAlmacenamiento, 0, longitudRuta);
	strcpy(rutaAlmacenamiento, primerBuffer);
	strcat(rutaAlmacenamiento, segundoBuffer);

  //borrar estas impresiones luego
	printf("Ruta de almacenamiento %s\n", rutaAlmacenamiento);

	if ((descriptorArchivo = open(rutaAlmacenamiento, O_CREAT | O_WRONLY, 00660)) == -1) {
		numeroAleatorio = rand() % 9999;	
		sprintf(cadenaNumero, "%d", numeroAleatorio);
		rutaAlmacenamiento = (char *)malloc(longitudRuta + 4);
		memset(rutaAlmacenamiento, 0, longitudRuta + 4);
		strcpy(rutaAlmacenamiento, primerBuffer);
		strcat(rutaAlmacenamiento, nombreArchivo);
		strcat(rutaAlmacenamiento, cadenaNumero);
		strcat(rutaAlmacenamiento, segundoBuffer);
		printf("Nueva Ruta de Almacenamiento %s\n", rutaAlmacenamiento);
	}
  return descriptorArchivo;
}
