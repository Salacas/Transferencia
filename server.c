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
#define MAXDATASIZE 1000

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
    char filename[FILENAMEMAXLEN], buf[MAXDATASIZE];
    int aux = 0, numbytes = 0;
    unsigned long filesize = 0, filesizeAux = 0, aux1 = 0;
    FILE *fd;
    time_t ahora = 0, antes = 0;
    
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
        
            fseek (fd , 0 , SEEK_END);
            filesize = ftell (fd);
            rewind (fd);
            //envio el tamaño
            if ( ( numbytes = send(new_fd, &filesize, sizeof(long), 0) ) == -1 )
            {
                perror("send");
                close(new_fd);
                fclose(fd);
                close(sockfd);
                exit(1);
            }

            aux1 = 0;
            filesizeAux = filesize;
            antes = time(NULL);
            while(aux1 != filesize)
            {
                //envio MAXDATASIZE bytes o, en su defecto,
                //la cantidad de bytes restantes.
                if(filesizeAux >MAXDATASIZE)
                {
                    fread(buf, 1,MAXDATASIZE, fd);
                    if ( ( numbytes = send(new_fd,buf, MAXDATASIZE, 0) ) == -1 )
                    {
                        perror("send");
                        close(new_fd);
                        fclose(fd);
                        close(sockfd);
                        exit(1);
                    }
                }
                else
                {
                    fread(buf, 1,filesizeAux, fd);
                    if ( ( numbytes = send(new_fd, buf, filesizeAux, 0) ) == -1 )
                    {
                        perror("send");
                        close(new_fd);
                        fclose(fd);
                        close(sockfd);
                        exit(1);
                    }
                }
                //aumento variables
                aux1 +=numbytes;
                filesizeAux-=numbytes;

                ahora = time(NULL);
                if(ahora != antes)//imprimo porcentaje cada 1 segundo
                {
                    system("clear");
                    printf("Enviando archivo\n");
                    printf("%ld%% completado\n", (100*aux1)/filesize);
                }
                antes = time(NULL);                
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
    int numbytes = 0, bufsize = 0, aux = 0;
    unsigned long filesize = 0, aux1 = 0;
    char buf[MAXDATASIZE], filename[FILENAMEMAXLEN];
    FILE *fd;
    time_t antes = 0, ahora = 0;

    system("clear");
    printf("Esperando al cliente...\n");
    //recibo cantidad de bytes a recibir en el proximo recv
    if ((numbytes = recv( new_fd , &bufsize, sizeof(int),  0 )) == -1  )
    {
        perror("recv");
        close(new_fd);
       	exit(1);
   	}
    //recibo el nombre del archivo en la pc del cliente
    if ((numbytes = recv( new_fd , buf, bufsize,  0 )) == -1  )
    {
        perror("recv");
        close(sockfd);
        close(new_fd);
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
        close(sockfd);
        close(new_fd);
       	exit(1);
   	}

    if (aux == 0 && (fd = fopen(filename, "w")) != NULL)
    {
        aux = 0;
        //envio confirmacion de que pude crear el archivo
        if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(sockfd);
            close(new_fd);
            exit(1);
        } 
        //recibo la cantidad de bytes del archivo a recibir
        if ((numbytes = recv( new_fd , &filesize, sizeof(long),  0 )) == -1  )
        {
            perror("recv");
            close(sockfd);
            fclose(fd);
            close(new_fd);
       	    exit(1);
   	    }
        
        aux1 = 0;
        antes = time(NULL);
        while(filesize > aux1)
        {
            //recibo MAXDATASIZE bytes o, en su defecto,
            //la cantidad de bytes restantes.
            if(aux1 < (filesize - MAXDATASIZE)){
                if ((numbytes = recv( new_fd , buf, MAXDATASIZE,  0 )) == -1  )
                {
                    perror("recv");
                    close(sockfd);
                    fclose(fd);
                    close(new_fd);
       	            exit(1);
   	            }
            }
            else{
                if ((numbytes = recv( new_fd , buf, (filesize - aux1),  0 )) == -1  )
                {
                    perror("recv");
                    close(sockfd);
                    fclose(fd);
                    close(new_fd);
       	            exit(1);
   	            }
            }
            fwrite(buf, 1, numbytes, fd);
            aux1 +=numbytes;

            ahora = time(NULL);
            if(ahora != antes)//imprimo porcentaje cada 1 segundo
            {
                system("clear");
                printf("Recibiendo archivo\n");
                printf("%ld%% completado\n", (100*aux1)/filesize);
            }
            antes = time(NULL);   
        }
        system("clear");

        if(filesize != aux1)
        {
            remove(filename);
            printf("Error al recibir el archivo\nIntente nuevamente\n");
        }
        else
        printf("Se ha recibido y guardado satisfactoriamente el archivo \"%s\"\n\n", filename);

        fclose(fd);        
    }
    else if(aux == 0)//si no pude crear el archivo
    {
        aux = -1;
        if ( ( numbytes = send(new_fd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(sockfd);
            close(new_fd);
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
