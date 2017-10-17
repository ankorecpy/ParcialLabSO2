#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 255

void extraerIndiceNombreArchivo(char * rutaArchivo);

typedef struct {
  char identificacionEstudiantes[BUFSIZE];
  char nombreArchivo[BUFSIZE];
  size_t tamanio;
}header;

int main(int argc, char * argv[]) {
  int s, bytesEnviados = 0, fd, n;
  struct sockaddr_in addr; /* Direccion de IPv4 */
  struct stat st;
  header h;
  char buf[BUFSIZ];

  if (argc != 4) {
    fprintf(stderr, "Uso: %s <IP> nombre_archivo\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  //Verificar que el archivo que se desea enviar exista
  if (stat(argv[3], &st) != 0) {
    perror("stat");
    exit(EXIT_FAILURE);
  }

  //Verificar que el archivo sea regular y se pueda leer
  if (!S_ISREG(st.st_mode) || !(st.st_mode & S_IRUSR)) {
    fprintf(stderr, "Archivo %s no valido\n", argv[2]);
    exit(EXIT_FAILURE);
  }

  //Abrir el archivo para lectura
  fd = open(argv[3], O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  //Crear el socket
  s = socket(PF_INET, SOCK_STREAM, 0);

  //Rellenar con ceros
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(2510);
  //addr.sin_addr.s_addr = INADDR_LOOPBACK; //Conexion a 127.0.0.1
  //Asignar la dirección AAA.BBB.CCC.DDD pasada por linea de comandos
  if (inet_aton(argv[1], &addr.sin_addr) == 0) {
    perror("inet_aton");
    exit(EXIT_FAILURE);
  }


  //Conectarse al servidor
  if (connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  //Configurar el encabezado
  memset(&h, 0, sizeof(header));
  strcpy(h.identificacionEstudiantes, argv[2]);
  strcpy(h.nombreArchivo, argv[3]); //Nombre del archivo
  h.tamanio = st.st_size; //Tamaño del archivo obtenido por stat

  extraerIndiceNombreArchivo(h.nombreArchivo);

  //Enviar el encabezado
  write(s, &h, sizeof(header));

  //enviar el contenido del archivo!!
  do {
    n = read(fd, buf, BUFSIZ);
    bytesEnviados += n;
    if (n > 0) {
      write(s, buf, n);
    } else {
      printf("Enviado %d de %d\n", bytesEnviados, st.st_size);
    }
  }while (n > 0);

  if(bytesEnviados == st.st_size) {
	  printf("Envio completado\n");
  } else {
	  printf("El archivo no se envió en su totalidad. Intente de nuevo");
  }

  close(fd); //Cerrar el archivo
  close(s); //Cerrar el socket

  exit(EXIT_SUCCESS);
}

void extraerIndiceNombreArchivo(char * rutaArchivo) {
	char * subcadena;	
	int indice;
	subcadena = strrchr(rutaArchivo, '/');
	if (subcadena != NULL) {
		indice = subcadena-rutaArchivo+1;
		subcadena = strchr(subcadena, rutaArchivo[indice]);
		strcpy(rutaArchivo, subcadena);
	}
}
