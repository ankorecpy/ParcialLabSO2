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
  int s;
  int c;
  struct sockaddr_in addr; /* Direccion de IPv4 */
  struct sockaddr_in c_addr; /* Direccion de IPv4 */

  socklen_t c_addr_len;
  int val, descriptorArchivo;
  int finished;
  pthread_t hilo_transferencia;

  //Crear el socket
  s = socket(PF_INET, SOCK_STREAM, 0);

  //Configurar el socket para reutilizar la direccion
  val = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  //Rellenar con ceros
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(2510);
  addr.sin_addr.s_addr = INADDR_ANY; //En todas las direcciones

  //Asociar la direccion INDARR_ANY y el puerto 2510 al socket
  if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  //Manifestar el deseo de recibir conexiones
  if (listen(s, 100) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  //Variable que controla el servidor
  finished = 0;

  while (!finished) {
    //Bloquearse hasta que se reciba una conexion
    printf("Esperando conexion\n");
    c_addr_len = sizeof(struct sockaddr_in);
    c = accept(s, (struct sockaddr *)&c_addr, &c_addr_len);
    if (c == -1) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    pthread_create(&hilo_transferencia, 0, transferencia, (void *)c);
  }

  close(s); //Cerrar el socket
  fprintf(stderr, "Server finished\n");
  exit(EXIT_SUCCESS);
}


//Funcion que realiza la comunicacion con el cliente
void * transferencia(void * conexion) {
  header h;
  int finished;
  char buf[BUFSIZ];
  int n, descriptorArchivo, longitudRuta, bytesEscritos;
  //Comunicacion con el cliente
  //1. Obtener la informacion del archivo a recibir
  if (read(conexion, &h, sizeof(header)) != sizeof(header)) {
    fprintf(stderr, "Error al recibir el encabezado\n");
    close(conexion);
    return;
  }
descriptorArchivo = crearArchivo(h.nombreArchivo, h.identificacionEstudiantes);


  //enviar a la salida estandar la informacion del cliente
  fprintf(stdout, "\nRecibiendo archivo %s - Enviado por %s - Tamanio %d\n\n", h.nombreArchivo, h.identificacionEstudiantes, h.tamanio);
  //2. Leer los datos del archivo enviados por el socket
  finished = 0;
  while(!finished) {
    memset(&buf, 0, BUFSIZ);
    n = read(conexion, buf, BUFSIZ); //Leer del socket
    if (n <= 0) {
      finished = 1;
    } else {
	printf("leido %d,\n\n%s\n", n, buf);
      bytesEscritos = write(descriptorArchivo, &buf, BUFSIZ); //Enviar a la salida estandar

    }
if (bytesEscritos == -1) {
	printf("\n\tERROR!!!!\n");
}
  }
  close(descriptorArchivo);
}

int crearArchivo(char * nombreArchivo, char * identificacionEstudiantes) {
  int longitudRuta, descriptorArchivo, numeroAleatorio;
  time_t tiempo = time(0);
  struct tm * local = localtime(&tiempo);
  char bufferFecha[20], * rutaAlmacenamiento, cadenaNumero[5] = "\0";

  longitudRuta = 11 + strlen(nombreArchivo) + strlen(identificacionEstudiantes) + 20 + 2;
  rutaAlmacenamiento = (char *)malloc(longitudRuta);
  memset(rutaAlmacenamiento, 0, longitudRuta);
  strcpy(rutaAlmacenamiento, "recibidos/");
  strcat(rutaAlmacenamiento, nombreArchivo);
  strcat(rutaAlmacenamiento, "_");
  strcat(rutaAlmacenamiento, identificacionEstudiantes);
  strcat(rutaAlmacenamiento, "_");
  strftime(bufferFecha, 20,"%d_%m_%Y_%H_%M_%S",local);
  strcat(rutaAlmacenamiento, bufferFecha);

  //borrar estas impresiones luego
  printf("Ruta de almacenamiento %s\n", rutaAlmacenamiento);
  printf("longitud %d\n", strlen(bufferFecha));
  printf("%s\n", nombreArchivo);

  if ((descriptorArchivo = open(rutaAlmacenamiento, O_CREAT | O_WRONLY, 00660)) == -1) {
  numeroAleatorio = rand() % 9999;	
  sprintf(cadenaNumero, "%d", numeroAleatorio);
  rutaAlmacenamiento = (char *)malloc(longitudRuta + 7);
  memset(rutaAlmacenamiento, 0, longitudRuta + 7);
  strcpy(rutaAlmacenamiento, "recibidos/");
  strcat(rutaAlmacenamiento, nombreArchivo);
  strcat(rutaAlmacenamiento, cadenaNumero);
  strcat(rutaAlmacenamiento, "_");
  strcat(rutaAlmacenamiento, identificacionEstudiantes);
  strcat(rutaAlmacenamiento, "_");
  strcat(rutaAlmacenamiento, bufferFecha);
  printf("Nueva Ruta de Almacenamiento %s\n", rutaAlmacenamiento);
  }

  return descriptorArchivo;
}
