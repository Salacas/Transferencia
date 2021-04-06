#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio_ext.h>
#include <termios.h>

#define PORT 3490 
#define MAXDATASIZE 1000
#define FILENAMEMAXLEN 100

void recibirArchivo(void);
void enviarArchivo(void);
void clean_stdin_fgets(void);

int sockfd = 0;

int main(int argc, char *argv[])
{
	struct in_addr addr;
	int numbytes = 0, opcion = 0;
	struct sockaddr_in their_addr;

	if (argc != 2) 
	{
		fprintf(stderr,"usage: client client_IP\n");
		exit(1);
	}
	
    if (inet_aton(argv[1], &addr) == 0) 
    {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    }

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0) ) == -1 ) 
	{
		perror("socket");
		exit(1);
	}
	
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(PORT);
	their_addr.sin_addr = addr;

	memset(&(their_addr.sin_zero), 0 , 8);
	
	if ( connect( sockfd , (struct sockaddr *)&their_addr,	sizeof(struct sockaddr)) == -1 ) 
	{
		perror("connect");
		close(sockfd);
		exit(1);
	}
    system("clear");
    //inicio de conexion--------------------------------------------------------------------
    opcion = 5;
    while(opcion != 0)
    {
        
        printf("Conexion con %s\n", argv[1]);
        printf("Esperando al servidor...\n");
        //recibo la opcion que eligio el servidor
        if ((numbytes = recv( sockfd , &opcion, sizeof(int),  0 )) == -1  )
        {
            perror("recv");
       	    exit(1);
   	    }
        system("clear");
        if(opcion == 1)
        recibirArchivo();
        else if(opcion == 2)
        enviarArchivo();
        else if(opcion == 0)
        printf("El servidor ha salido del programa\n");
    }

    return 0;
}

void recibirArchivo(void)
{
    int numbytes = 0, bufsize = 0, aux = 0, porcentaje = -10, i = 0;
    unsigned long filesize = 0;
    char buf[MAXDATASIZE], filename[FILENAMEMAXLEN], c; 
    FILE *fd;

    system("clear");
    printf("El server eligio enviarle un archivo\n");
    printf("Esperando al servidor...\n");
    //recibo cantidad de bytes a recibir en el proximo recv
    if ((numbytes = recv( sockfd , &bufsize, sizeof(int),  0 )) == -1  )
    {
        perror("recv");
        close(sockfd);
       	exit(1);
   	}
    //recibo el nombre del archivo en la pc del server
    if ((numbytes = recv( sockfd , buf, bufsize,  0 )) == -1  )
    {
        perror("recv");
        close(sockfd);
       	exit(1);
   	}
    buf[numbytes]='\0';
    system("clear");
    printf("El server le enviara el archivo llamado \"%s\"\n", buf);

    do{
    printf("Ingrese el nombre del archivo a guardar(incluya la extension): ");
    clean_stdin_fgets();
    fgets(filename, FILENAMEMAXLEN, stdin);
    filename[strcspn(filename, "\n")] = '\0';//le saco el \n

    if(fopen(filename, "r")!= NULL)
    printf("Ya existe un archivo con ese nombre en la carpeta\n");
    }while(fopen(filename, "r")!= NULL);

    //recibo 0 si el server pudo abrir el archivo
    //-1 en caso de no poder abrirlo
    if ((numbytes = recv( sockfd , &aux, sizeof(int),  0 )) == -1  )
    {
        perror("recv");
        close(sockfd);
       	exit(1);
   	}

    if (aux == 0 && (fd = fopen(filename, "w")) != NULL)
    {
        aux = 0;
        //envio confirmacion de que pude crear el archivo
        if ( ( numbytes = send(sockfd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(sockfd);
            exit(1);
        } 
        //recibo la cantidad de bytes del archivo a recibir
        if ((numbytes = recv( sockfd , &filesize, sizeof(long),  0 )) == -1  )
        {
            perror("recv");
            fclose(fd);
            close(sockfd);
       	    exit(1);
   	    }

        porcentaje = -10;
        //recibo byte por byte
        for(i = 0; i < filesize; i++)
        {
            if ((numbytes = recv( sockfd , &c, sizeof(char),  0 )) == -1  )
            {
                perror("recv");
                fclose(fd);
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
        if ( ( numbytes = send(sockfd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(sockfd);
            exit(1);
        } 
        printf("Error al crear el archivo \"%s\"\n", filename);
    }  
}

void enviarArchivo(void)
{
    char filename[FILENAMEMAXLEN], c;
    int aux = 0, numbytes = 0, porcentaje = 0, i = 0;
    unsigned long filesize = 0, filesizeAux = 0;
    FILE *fd;

    system("clear");
    printf("El server eligio recibir un archivo suyo\n");

    do{
    printf("Introduzca el nombre del archivo a enviar(incluir extension): ");
    clean_stdin_fgets();
    fgets(filename, FILENAMEMAXLEN, stdin);
    filename[strcspn(filename, "\n")] = '\0';//le saco el \n

    if(fopen(filename, "r")== NULL)
    printf("No existe un archivo con ese nombre\n");
    }while(fopen(filename, "r")== NULL);

    printf("Esperando al server\n");

    aux = strlen(filename);
    //envio cantidad de caracteres de filename
    if ( ( numbytes = send(sockfd, &aux, sizeof(int), 0) ) == -1 )
    {
        perror("send");
        close(sockfd);
        exit(1);
    }
    //envio filename
    if ( ( numbytes = send(sockfd, filename, strlen(filename), 0) ) == -1 )
    {
        perror("send");
        close(sockfd);
        exit(1);
    }
    
    if((fd = fopen(filename, "r")) != NULL)//abro el archivo
    {
        aux = 0;
        //aviso al server que se abrio correctamente el archivo
        if ( ( numbytes = send(sockfd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            fclose(fd);
            close(sockfd);
            exit(1);
        }

        //recibo 0 si el server pudo abrir el archivo correctamente
        //-1 en caso de error
        if ((numbytes = recv( sockfd , &aux, sizeof(int),  0 )) == -1  )
        {
            perror("recv");
            close(sockfd);
       	    exit(1);
   	    }

        if(aux == 0)//envio el archivo
        {
            //calculo el tamaño del archivo
            for(filesize = -1; feof(fd) == 0; filesize++)
            fgetc(fd);
            //envio el tamaño
            if ( ( numbytes = send(sockfd, &filesize, sizeof(long), 0) ) == -1 )
            {
                perror("send");
                fclose(fd);
                close(sockfd);
                exit(1);
            }

            fseek(fd,0,SEEK_SET);
            porcentaje = -10;
            for(i = 0; i < filesize; i++)
            {
                c = fgetc(fd);
                if ( ( numbytes = send(sockfd, &c, sizeof(char), 0) ) == -1 )
                {
                    perror("send");
                    fclose(fd);
                    close(sockfd);
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
        else if(aux == -1)//error en el server
        printf("El cliente no pudo crear el archivo a guardar\n");

        fclose(fd);
    }
    else//si no pude abrir el archivo
    {
        aux = -1;
        if ( ( numbytes = send(sockfd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            fclose(fd);
            close(sockfd);
            exit(1);
        }
        printf("Error al abrir el archivo \"%s\"\n", filename);
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