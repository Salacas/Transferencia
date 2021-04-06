#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <stdio_ext.h>
#include <termios.h>

#define MYPORT 3490
#define BACKLOG 0
#define FILENAMEMAXLEN 100
#define MAXDATASIZE 100

int sockfd = 0, new_fd = 0;

void enviarArchivo(void);
void recibirArchivo(void);
void clean_stdin_fgets(void);
void clean_stdin(void);

int main( void )
{
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	socklen_t sin_size;
	int yes = 1, numbytes = 0, opcion = 0;
    char buf[MAXDATASIZE];
	if ( ( sockfd = socket( AF_INET , SOCK_STREAM , 0 ) ) == -1) 
	{
		perror("socket");
		exit(1);
	}
	
	if ( setsockopt( sockfd , SOL_SOCKET , SO_REUSEADDR , &yes , sizeof(int) ) == -1) 
	{
		perror("setsockopt");
		close(sockfd);
		exit(1);
	}
	
	my_addr.sin_family = AF_INET; 
	my_addr.sin_port = htons( MYPORT ); 
	my_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8);

	if ( bind( sockfd , (struct sockaddr *)&my_addr, sizeof( struct sockaddr ) ) == -1 ) 
	{
		perror("bind");
		close(sockfd);
		exit(1);
	}

	if ( listen( sockfd , BACKLOG ) == -1 ) 
	{
		perror("listen");
		close(sockfd);
		exit(1);
	}

	printf("Esperando conexion...\n");
	
	sin_size = sizeof( struct sockaddr_in );

	if ( ( new_fd = accept( sockfd , (struct sockaddr *)&their_addr, &sin_size)) == -1)
	{ 
		perror("accept");
		close(sockfd);
		exit(1);
	}

	//inicio de conexion-------------------------------------------------------------------
    opcion = 5;
    while(opcion != 0)
    {
        system("clear");
        printf("Conexion desde %s\n", inet_ntoa(their_addr.sin_addr));
        printf("Ingrese 1 si quiere enviar un archivo al cliente\n");
        printf("Ingrese 2 si quiere recibir un archivo del cliente\n");
        printf("Ingrese 0 si quiere salir del programa\n");
        opcion = getchar() - 48;//tomo un caracter de stdin
        clean_stdin();
        //envio la opcion elegida al cliente
        if(opcion >=0 && opcion <=2)
        {
            if ( ( numbytes = send(new_fd, &opcion, sizeof(int), 0) ) == -1 )
            {
                perror("send");
                close(new_fd);
                close(sockfd);
                exit(1);
            }
        }

        if(opcion == 1)
        {
            enviarArchivo();
        }
        else if(opcion == 2)
        {
            recibirArchivo();
        }
        else if(opcion == 0)
        printf("Saliendo del programa...\n");
    }

    close(new_fd);
    close(sockfd);
    
    return 0;
}

void enviarArchivo(void)
{
    char filename[FILENAMEMAXLEN], buf[MAXDATASIZE], c;
    int aux = 0, numbytes = 0, porcentaje = -10, i = 0;
    unsigned long filesize = 0, filesizeAux = 0;
    FILE *fd;

    system("clear");

    do{
    printf("Introduzca el nombre del archivo a enviar(incluir extension): ");
    clean_stdin_fgets();
    fgets(filename, FILENAMEMAXLEN, stdin);
    filename[strcspn(filename, "\n")] = '\0';//le saco el \n

    if(fopen(filename, "r")== NULL)
    printf("No existe un archivo con ese nombre\n");
    }while(fopen(filename, "r")== NULL);

    printf("Esperando al cliente\n");

    aux = strlen(filename);
    //envio cantidad de caracteres de filename
    if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
    {
        perror("send");
        close(new_fd);
        close(sockfd);
        exit(1);
    }
    //envio filename
    if ( ( numbytes = send(new_fd, filename, strlen(filename), 0) ) == -1 )
    {
        perror("send");
        close(new_fd);
        close(sockfd);
        exit(1);
    }
    
    if((fd = fopen(filename, "r")) != NULL)//abro el archivo
    {
        aux = 0;
        //aviso al cliente que se abrio correctamente el archivo
        if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(new_fd);
            fclose(fd);
            close(sockfd);
            exit(1);
        }

        //recibo 0 si el cliente pudo abrir el archivo correctamente
        //-1 en caso de error
        if ((numbytes = recv( new_fd , &aux, sizeof(int),  0 )) == -1  )
        {
            perror("recv");
            close(new_fd);
            close(sockfd);
       	    exit(1);
   	    }

        if(aux == 0)//envio el archivo
        {
            //calculo el tamaño del archivo
            for(filesize = -1; feof(fd) == 0; filesize++)
            fgetc(fd);
            //envio el tamaño
            if ( ( numbytes = send(new_fd, &filesize, sizeof(long), 0) ) == -1 )
            {
                perror("send");
                fclose(fd);
                close(sockfd);
                close(new_fd);
                exit(1);
            }

            fseek(fd,0,SEEK_SET);
            porcentaje = -10;
            for(i = 0; i < filesize; i++)
            {
                c = fgetc(fd);
                if ( ( numbytes = send(new_fd, &c, sizeof(char), 0) ) == -1 )
                {
                    perror("send");
                    fclose(fd);
                    close(sockfd);
                    close(new_fd);
                    exit(1);
                }
                if(i %(filesize /10) == 0 && porcentaje < 100)
                {
                    porcentaje+=10;
                    system("clear");
                    printf("Enviando archivo\n");
                    printf("%d%% completado\n", porcentaje);
                }
            }
            printf("El archivo \"%s\" se ha enviado satisfactoriamente\n\n", filename);
        }
        else if(aux == -1)//error en el cliente
        printf("El cliente no pudo crear el archivo a guardar\n");

        fclose(fd);
    }
    else//si no pude abrir el archivo
    {
        aux = -1;
        if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(new_fd);
            fclose(fd);
            close(sockfd);
            exit(1);
        }
        printf("Error al abrir el archivo \"%s\"\n", filename);
    }
}

void recibirArchivo(void)
{
    int numbytes = 0, bufsize = 0, aux = 0, porcentaje = 0, i = 0;
    unsigned long filesize = 0;
    char buf[FILENAMEMAXLEN], filename[FILENAMEMAXLEN], c; 
    FILE *fd;
    //recibo cantidad de bytes a recibir en el proximo recv
    if ((numbytes = recv( new_fd , &bufsize, sizeof(int),  0 )) == -1  )
    {
        perror("recv");
        close(sockfd);
        close(new_fd);
       	exit(1);
   	}
    //recibo el nombre del archivo en la pc del cliente
    if ((numbytes = recv( new_fd , buf, bufsize,  0 )) == -1  )
    {
        perror("recv");
        close(new_fd);
        close(sockfd);
       	exit(1);
   	}
    buf[numbytes]='\0';
    system("clear");
    printf("El cliente le enviara el archivo llamado \"%s\"\n", buf);

    do{
    printf("Ingrese el nombre del archivo a guardar(incluya la extension): ");
    clean_stdin_fgets();
    fgets(filename, FILENAMEMAXLEN, stdin);
    filename[strcspn(filename, "\n")] = '\0';//le saco el \n

    if(fopen(filename, "r")!= NULL)
    printf("Ya existe un archivo con ese nombre en la carpeta\n");

    }while(fopen(filename, "r")!= NULL);

    //recibo 0 si el cliente pudo abrir el archivo
    //-1 en caso de no poder abrirlo
    if ((numbytes = recv( new_fd , &aux, sizeof(int),  0 )) == -1  )
    {
        perror("recv");
        close(new_fd);
        close(sockfd);
       	exit(1);
   	}

    if (aux == 0 && (fd = fopen(filename, "w")) != NULL)
    {
        aux = 0;
        //envio confirmacion de que pude crear el archivo
        if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(new_fd);
            close(sockfd);
            exit(1);
        } 
        //recibo la cantidad de bytes del archivo a recibir
        if ((numbytes = recv( new_fd , &filesize, sizeof(long),  0 )) == -1  )
        {
            perror("recv");
            fclose(fd);
            close(new_fd);
            close(sockfd);
       	    exit(1);
   	    }
        
        porcentaje = -10;
        //recibo byte por byte
        for(i = 0; i < filesize; i++)
        {
            if ((numbytes = recv( new_fd , &c, sizeof(char),  0 )) == -1  )
            {
                perror("recv");
                fclose(fd);
                close(new_fd);
                close(sockfd);
       	        exit(1);
   	        }
            fputc(c, fd);
            //imprimo un porcentaje del progreso
            if(i %(filesize /10) == 0 && porcentaje < 100)
            {
                porcentaje+=10;
                system("clear");
                printf("Recibiendo archivo\n");
                printf("%d%% completado\n", porcentaje);
            }
        }
        fputc(EOF, fd);
        fclose(fd);
        printf("Se ha recibido y guardado satisfactoriamente el archivo \"%s\"\n\n", filename);
    }
    else if(aux == 0)//si no pude crear el archivo
    {
        aux = -1;
        if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(new_fd);
            close(sockfd);
            exit(1);
        } 
        printf("Error al crear el archivo \"%s\"\n", filename);
    }  
}

void clean_stdin_fgets(void)
{
	//limpia el stdin para usarlo con fgets
	int stdin_copy = dup(STDIN_FILENO);
    tcdrain(stdin_copy);
    tcflush(stdin_copy, TCIFLUSH);
    close(stdin_copy);
}

void clean_stdin(void)
{
    //limpia el stdin para usarlo con getchar
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}