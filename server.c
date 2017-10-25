/**
 * @author German David Garcia <garciagd@unicauca.edu.co> Alejandro Mendez Astudillo <victoralemendez@unicauca.edu.co>
 * 
 * @brief Recibe y almacena archivos enviados por los clientes
 *
 * */
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

/** 
 * @brief Crea un archivo en la ruta especificada con permisos de lectura y escritura para el usuario y su grupo, ningun permiso para otros, retorna el descriptor del archivo creado.
 *
 * @param ruta direccion del archivo a crear
 * */
int crearArchivoCon(char * ruta);

/**
 * @brief Crea y configura la ruta del archivo a crear, el archivo se crea en el directorio /recibidos que se encuentra ubicado en directorio actual, hace uso de la funcion crearArchivoCon(char * ruta), en caso de ya existir el archivo se hace uso de la funcion crearArchivoNumero(char * nombreArchivo, char * identificacionEstudiantes), hasta que sea posible la creacion del archivo. 
 *
 * @param nombreArchivo Cadena de caracteres llena con el nombre del archivo y/o su extension
 * @param identificacionEstudiantes Cadena de caracteres llena con el/los codigos de los estudiantes. 
 * */
int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes);

/**
 * @brief Crea un archivo en el directorio /recibidos con el nombre <nombreArchivo>NumeroAleatorio[<extension>]<complementoDatos> 
 *
 * @param nombreArchivo Cadena de caracteres llena con el nombre del archivo y/o su extension
 * @param identificacionEstudiantes Cadena de caracteres llena con el/los codigos de los estudiantes.
 * */
int crearArchivoNumero(char * nombreArchivo, char * complementoDatos);

/**
 * @brief funcion encargada de procesas los archivos recibidos.
 *
 * @param * conexion recibida
 * */
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
		printf("Esperando conexion\n");
		c_addr_len = sizeof(struct sockaddr_in);
                //Bloquearse hasta que se reciba una conexion
		conexion = accept(_socket, (struct sockaddr *)&c_addr, &c_addr_len);
		if (conexion == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		pthread_create(&hilo_transferencia, 0, transferencia, (void *)conexion);
	}
	//cerrar el socket
	close(_socket); 
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
        //Obtener la informacion del archivo a recibir
	if (read(conexion, &encabezado, sizeof(header)) != sizeof(header)) {
		fprintf(stderr, "Error al recibir el encabezado\n");
		close(conexion);
		return;
	}
	descriptorArchivo = crearArchivo(encabezado.nombreArchivo, encabezado.identificacionEstudiantes);

        //enviar a la salida estandar la informacion del cliente
	fprintf(stdout, "\nRecibiendo archivo %s - Enviado por %s - Tamanio %d\n\n", encabezado.nombreArchivo, encabezado.identificacionEstudiantes, encabezado.tamanio);
        //Leer los datos del archivo enviados por el socket
	while(!finalizacion) {
		memset(&buf, 0, BUFSIZ);
		//leer del socket
		n = read(conexion, buf, BUFSIZ); 
		if (n <= 0) {
			finalizacion = 1;
		} else {
			bytesEscritos = write(descriptorArchivo, &buf, n);
		}
	}
	close(descriptorArchivo);
}

int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes) {
	int longitudRuta, descriptorArchivo = 0, tamanioPrimerBuffer = strlen(nombreArchivo) + 12, tamanioSegundoBuffer;
	time_t tiempo = time(0);
	struct tm * local = localtime(&tiempo);
	char bufferFecha[20], * rutaAlmacenamiento, * segundoBuffer, * primerBuffer;

	strftime(bufferFecha, 20,"%d_%m_%Y_%H_%M_%S", local);
	tamanioSegundoBuffer = strlen(bufferFecha) + strlen(identificacionEstudiantes) + 1;

	primerBuffer = (char *)malloc(tamanioPrimerBuffer);
	memset(primerBuffer, 0, tamanioPrimerBuffer);
	sprintf(primerBuffer, "recibidos/%s", nombreArchivo);

	segundoBuffer = (char *)malloc(tamanioSegundoBuffer);
	memset(segundoBuffer, 0, tamanioSegundoBuffer);
	sprintf(segundoBuffer, "_%s_%s", identificacionEstudiantes, bufferFecha);

	longitudRuta = tamanioPrimerBuffer + tamanioSegundoBuffer;
	rutaAlmacenamiento = (char *)malloc(longitudRuta);
	memset(rutaAlmacenamiento, 0, longitudRuta);
	sprintf(rutaAlmacenamiento, "%s%s", primerBuffer, segundoBuffer);

	descriptorArchivo = crearArchivoCon(rutaAlmacenamiento);

	while (descriptorArchivo == -1) {
		descriptorArchivo = crearArchivoNumero(primerBuffer, segundoBuffer);
	}
	return descriptorArchivo;
}

int crearArchivoNumero(char * nombreArchivo, char * complementoDatos) {
	char * nombre, * extension, cadenaNumero[10], *nuevaCadena;
	int indice = 0, numeroAleatorio = 0, longitud = 0;
	memset(&cadenaNumero, 0, 10);
	numeroAleatorio = rand() % 999999999;
	sprintf(cadenaNumero, "%d", numeroAleatorio);
	extension = strchr(nombreArchivo, '.');
	longitud = strlen(cadenaNumero) + strlen(complementoDatos);
	longitud++;

	if (extension != NULL) {
		indice = extension - nombreArchivo;
		nombre = (char *)malloc(indice);
		strncpy(nombre, nombreArchivo, indice);		
		longitud += strlen(nombre) + strlen(extension);
		nuevaCadena = (char *)malloc(longitud);
		memset(nuevaCadena, 0, longitud);
		sprintf(nuevaCadena, "%s_%s%s%s", nombre, cadenaNumero, extension, complementoDatos);
	} else {
		longitud += strlen(nombreArchivo);
		nuevaCadena = (char *)malloc(longitud);
		memset(nuevaCadena, 0, longitud);
		sprintf(nuevaCadena, "%s_%s%s", nombreArchivo, cadenaNumero, complementoDatos);
	}
	return crearArchivoCon(nuevaCadena);
}

int crearArchivoCon(char * ruta) {
	int descriptorArchivo = 0;
	sem_wait(&mutex);
	descriptorArchivo = open(ruta, O_CREAT | O_EXCL | O_WRONLY, 00660);
	sem_post(&mutex);
	return descriptorArchivo;
}

