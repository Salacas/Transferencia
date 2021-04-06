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
#include <time.h>

#define PORT 3490 
#define MAXDATASIZE 1000
#define FILENAMEMAXLEN 100

void recibirArchivo(void);
void enviarArchivo(void);
void clean_stdin_fgets(void);
void clean_stdin(void);

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
    int numbytes = 0, bufsize = 0, aux = 0, getcharAux = 0;
    unsigned long filesize = 0, aux1 = 0;
    char buf[MAXDATASIZE], filename[FILENAMEMAXLEN];
    FILE *fd;
    time_t antes = 0, ahora = 0;

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
        {
            printf("Ya existe un archivo con ese nombre en la carpeta\n");
            printf("Desea sobreescribirlo?(s/n)\n");

            while(getcharAux != 'n'&& getcharAux != 's')
            {
                clean_stdin_fgets();
                fgets(buf, 4, stdin);
                getcharAux = buf[0];
                if(getcharAux == 's')
                break;
            }
        }
        if(getcharAux == 's')
        break;
        
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
        aux1 = 0;
        antes = time(NULL);
        while(filesize > aux1)
        {
            //recibo MAXDATASIZE bytes o, en su defecto,
            //la cantidad de bytes restantes.
            if(aux1 < (filesize - MAXDATASIZE)){
                if ((numbytes = recv( sockfd , buf, MAXDATASIZE,  0 )) == -1  )
                {
                    perror("recv");
                    fclose(fd);
                    close(sockfd);
       	            exit(1);
   	            }
            }
            else{
                if ((numbytes = recv( sockfd , buf, (filesize - aux1),  0 )) == -1  )
                {
                    perror("recv");
                    fclose(fd);
                    close(sockfd);
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
    char filename[FILENAMEMAXLEN], buf[MAXDATASIZE];
    int aux = 0, numbytes = 0;
    unsigned long filesize = 0, filesizeAux = 0, aux1 = 0;
    FILE *fd;
    time_t ahora = 0, antes = 0;
    
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
            close(sockfd);
            fclose(fd);
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
            fseek (fd , 0 , SEEK_END);
            filesize = ftell (fd);
            rewind (fd);
            //envio el tamaño
            if ( ( numbytes = send(sockfd, &filesize, sizeof(long), 0) ) == -1 )
            {
                perror("send");
                close(sockfd);
                fclose(fd);
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
                    if ( ( numbytes = send(sockfd,buf, MAXDATASIZE, 0) ) == -1 )
                    {
                        perror("send");
                        close(sockfd);
                        fclose(fd);
                        exit(1);
                    }
                }
                else
                {
                    fread(buf, 1,filesizeAux, fd);
                    if ( ( numbytes = send(sockfd, buf, filesizeAux, 0) ) == -1 )
                    {
                        perror("send");
                        close(sockfd);
                        fclose(fd);
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
            system("clear");
            printf("El archivo \"%s\" se ha enviado satisfactoriamente\n\n", filename);
        }
        else if(aux == -1)//error en el server
        printf("El server no pudo crear el archivo a guardar\n");

        fclose(fd);
    }
    else//si no pude abrir el archivo
    {
        aux = -1;
        if ( ( numbytes = send(sockfd, &aux, sizeof(int), 0) ) == -1 )
        {
            perror("send");
            close(sockfd);
            fclose(fd);
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

void clean_stdin(void)
{
    //limpia el stdin para usarlo con getchar
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}