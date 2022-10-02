#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define HOST "127.0.0.1"
#define PORT 65432

int main(int argc, char **argv){
  //Dati necessari per scrittura socket
  int fd_skt = 0;
  struct sockaddr_in serv_addr;
  //Comunico alla socket
  if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    termina("Errore creazione socket.\n");
  // assegna indirizzo
  serv_addr.sin_family = AF_INET;
  //Network order
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(HOST);
  // apriamo connessione
    if (connect(fd_skt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
      if(close(fd_skt) < 0)
        perror("Errore chiusura socket: Client.\n");
      termina("Errore apertura connessione: Client\n");
    }
    
  
  //Chiediamo di stampare tutte le coppie (richiesta tipo 2)
  
  if (argc == 1){ //solo "client" su riga di comando
    int tipo = htonl(2);
    int e = writen(fd_skt, &tipo, sizeof(tipo));
    if(e != sizeof(int)) 
      chiudi_socket(fd_skt, "Errore writen (Client: 2)\n");
    
    //Mi faccio dire quante coppie esistono.
    int quanti;
    int temp = readn(fd_skt, &quanti, sizeof(int));
    if (temp != sizeof(int))
      chiudi_socket(fd_skt, "Errore readn (Client: 2.1)\n");
    quanti = ntohl(quanti);

    for(int i=0; i<quanti; i++){
      //Leggo nomeFile
      int dim;
      temp = readn(fd_skt, &dim, sizeof(int));
      if (temp != sizeof(int))
        chiudi_socket(fd_skt, "Errore readn (Client: 2.2)\n");
      dim = ntohl(dim);
      
      char *name = malloc((dim+1) * sizeof(char));
      temp = readn(fd_skt, name, dim);
      if (temp != dim)
        chiudi_socket(fd_skt, "Errore readn (Client: 2.3)\n");
      name[dim] = '\0';
      //Leggo la somma (come stringa)
      temp = readn(fd_skt, &dim, sizeof(int));
      if (temp != sizeof(int))
        chiudi_socket(fd_skt, "Errore readn (Client: 2.4)\n");
      dim = ntohl(dim);
      //fprintf(stderr, "Dim: %d\n", dim);
      char *sum = malloc((dim + 1) * sizeof(char));
      temp = readn(fd_skt, sum, dim);
      if (temp != dim)
        chiudi_socket(fd_skt, "Errore readn (Client: 2.5)\n");
      sum[dim] = '\0';
      //Finalmente stampiamo le coppie 
      fprintf(stdout, "%s : %s\n", name, sum);
      free(name);
      free(sum);
    }
  }
    
  else{  //Chiediamo coppie specifiche (richiesta tipo 3)
    int tipo = htonl(3);
    int e = writen(fd_skt, &tipo, sizeof(tipo));
    if(e != sizeof(int))
      chiudi_socket(fd_skt, "Errore writen (Client: 3)\n");
    
    
    //Controllo quali e quanti numeri sono validi, tra quelli 
    //passati sulla linea di comando.
    
    int validi = 0;
    char **a = malloc(argc*sizeof(char *));
    for(int i=1; i<argc; i++){
      if(strcmp(argv[i], "0") == 0){ //Controllo aggiuntivo
        a[validi] = argv[i];
        continue;
      }
      long num = atol(argv[i]); //uso atol per controllare la validità
      if (num == 0) {//Non valido
        fprintf(stderr, "%s non è un numero valido\n", argv[i]);
        continue;
      } 
      a[validi] = argv[i]; //inserisco la stringa, non il long
      validi += 1;
    }
    a = realloc(a, validi*sizeof(char *));

    //Comunico al collector quanti numeri dovrà leggere
    int temp = htonl(validi);
    e = writen(fd_skt, &temp, sizeof(temp));
    if(e != sizeof(int))
      chiudi_socket(fd_skt, "Errore writen(Client: 3.1)\n");
      
    //Invio i numeri
    for(int i=0; i<validi; i++){
      //Invio dimensione numero
      temp = htonl(strlen(a[i]));
      e = writen(fd_skt, &temp, sizeof(temp));
      if(e != sizeof(int)) 
        chiudi_socket(fd_skt, "Errore writen(Client: 3.2)\n");
      //Invio il numero al collector
      e = writen(fd_skt, a[i], strlen(a[i]));
      if(e != strlen(a[i]))
        chiudi_socket(fd_skt, "Errore writen(Client: 3.3)\n");
      }

    //Ricevo le coppie
    int dim;
    for(int i=0; i<validi; i++){
      temp = readn(fd_skt, &dim, sizeof(int)); //Dim nomeFile
      if (temp != sizeof(int))
        chiudi_socket(fd_skt, "Errore readn (Client: 3.4)\n");
      dim = ntohl(dim);
      char *name = malloc((dim+1) * sizeof(char)); //nomeFile
      temp = readn(fd_skt, name, dim);
      if (temp != dim)
        chiudi_socket(fd_skt, "Errore readn (Client: 3.5)\n");
      name[dim] = '\0';
      //Se ricevo Nessun File non leggo la somma, ma salto 
      //all'iterazione dopo.
      if(strcmp(name, "Nessun File") == 0){ 
        fprintf(stdout, "%s\n", name);
        continue;
      }
      temp = readn(fd_skt, &dim, sizeof(int)); //Dim Somma
      if (temp != sizeof(int))
        chiudi_socket(fd_skt, "Errore readn (Client: 3.6)\n");
      dim = ntohl(dim);
      char *sum = malloc((dim+1) * sizeof(char)); //Somma
      temp = readn(fd_skt, sum, dim);
      if (temp != dim)
        chiudi_socket(fd_skt, "Errore readn (Client: 3.7)\n");
      sum[dim] = '\0';
      fprintf(stdout, "%s : %s\n", name, sum);
      free(name);
      free(sum);
    }
    
    free(a);
    if(close(fd_skt)<0)
      perror("Errore chiusura socket\n");
  }
  return 0;
}