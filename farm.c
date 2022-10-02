#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define HOST "127.0.0.1"
#define PORT 65432

typedef struct {
  int *letti;  //indice nel buffer
  char **buffer; // Buf coi nomi dei file (char *)
	pthread_mutex_t *mutex;
  sem_t *sem_free_slots;
  sem_t *sem_data_items;
	int *size;
} dati;

//Worker Thread - Nomi DAL Buffer

void *tbody (void *v){
  //Dati necessari per lettura file
	dati *d = (dati *) v;
	int j = 0;
	long int num;
	long res = 0;
  //Dati necessari per scrittura socket
  int fd_skt = 0;
  struct sockaddr_in serv_addr;
  int temp;
  //Leggo dal buffer
	while(true){
		xsem_wait(d->sem_data_items, QUI);
		xpthread_mutex_lock(d->mutex, QUI);
		char *name = d->buffer[*(d->letti) % *(d->size)];
		if(strcmp(name, ":STOP:") == 0){
			xpthread_mutex_unlock(d->mutex, QUI);
			xsem_post(d->sem_data_items, QUI); //Rileggiamo "STOP"
			break;
		}
		*(d->letti) += 1;
		xpthread_mutex_unlock(d->mutex, QUI);
		xsem_post(d->sem_free_slots, QUI);
		//LAVORO SUL FILE
		FILE *f = fopen(name, "rb");
		if(f == NULL) {
			fprintf(stderr, "Errore apertura file: %s\n", name);
			continue;
		}
    j = 0;
    res = 0;
		while(true){ 	//Leggo i numeri dal file
			int e = fread(&num, sizeof(long), 1, f);
			if(e != 1)	break;
			res += (num * j);
			j++;
		}
    
    //Comunico alla socket
    if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      fclose(f);
      xtermina("Errore creazione socket.\n", __LINE__, __FILE__);
    }
      
    // assegna indirizzo
    serv_addr.sin_family = AF_INET;
    //Network order
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    // apriamo connessione
    if (connect(fd_skt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)  
      chiudi_tutto(fd_skt, f, "Errore connessione socket.\n");
    //Inviamo il tipo di richiesta (1: dai workers)
    temp = htonl(1);
    int e = writen(fd_skt, &temp, sizeof(temp));
    if(e != sizeof(int)) 
      chiudi_tutto(fd_skt, f, "Errore tipo di richiesta.\n");
    //Invio la dimensione di nomeFile
    temp = htonl(strlen(name));
    e = writen(fd_skt, &temp, sizeof(temp));
    if(e != sizeof(int))  
      chiudi_tutto(fd_skt, f, "Errore dimensione nomeFile.\n");
    //Inviamo nomeFile, carattere per carattere
    e = writen(fd_skt, name, strlen(name));
    if(e != strlen(name))  
      chiudi_tutto(fd_skt, f, "Errore nomeFile (stringa)\n");
    //"Preparo" la somma, da long ---> stringa 
    char *buff;
    int sommaLen = asprintf(&buff, "%li", res);
    if (sommaLen == -1)  
      chiudi_tutto(fd_skt, f, "Errore asprintf (somma -> string)\n");
    //Invio la dimensione della somma
    temp = htonl(sommaLen);
    e = writen(fd_skt, &temp, sizeof(temp));
    if(e != sizeof(int)){
      free(buff);
      chiudi_tutto(fd_skt, f, "Errore dimensione somma.\n");
    } 
    //Invio la stringa "somma"
    e = writen(fd_skt, buff, strlen(buff));
    if(e != strlen(buff)){
      free(buff);
      chiudi_tutto(fd_skt, f, "Errore somma (invio stringa)\n");
    }
    free(buff);
		if(close(fd_skt) < 0)
      perror("Errore chiusura Socket\n");
    fclose(f);
	}
	pthread_exit(NULL);
}

//Handler Thread - Gestisce SIGINT

volatile sig_atomic_t sig = false;

void gestore(int s){
	if (s == SIGINT)
	sig = true;
	fprintf(stderr, "\nRicevuto SIGINT\n");
}

int main(int argc, char *argv[])
{
  // controlla numero argomenti
  if(argc<2) {
    printf("Uso: %s file [file ...] \n",argv[0]);
    return 1;
  }
  
	//STRUCT GESTIONE SIGINT
  
	struct sigaction sa;
  sigaction(SIGINT, NULL, &sa); // inizializzo sa
  sa.sa_handler = gestore;       // fornisce handler
  sigaction(SIGINT, &sa, NULL);
	
	//INIZIALIZZO I PARAMETRI OPZIONALI	
	
	int opt;
	int numt = 4;
	int buf_size = 8;
	int delay = 0;
	int numopt = 0;
	while((opt = getopt(argc, argv, "n:q:t:")) != -1){
		switch(opt) {
			case 'n':
				numt = atoi(optarg);
				numopt += 1;
				break;
			case 'q':
				buf_size = atoi(optarg);
				numopt += 1;
				break;
			case 't':
				delay = atoi(optarg);
				numopt += 1;
				break;
			default:
			break;
		}
	}

	//INIZIALIZZO THREAD E STRUCT

	char *buffer[buf_size]; // buffer
	
	//--------semafori e mutex

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	sem_t sem_free_slots, sem_data_items;
  xsem_init(&sem_free_slots,0,buf_size,QUI);
  xsem_init(&sem_data_items,0,0,QUI);
	
	//--------thread e struct
	
	pthread_t t[numt];
	dati d;
	int letti = 0;
	d.buffer = buffer;
	d.letti = &letti;
	d.sem_data_items = &sem_data_items;
  d.sem_free_slots = &sem_free_slots;
	d.mutex = &mutex;
	d.size = &buf_size;
	for(int i=0; i<numt; i++)
		xpthread_create(&t[i], NULL, &tbody, &d, QUI);

	//Main Thread - Scrive NEL buffer

	int messi = 0;
	for(int i=(2*numopt)+1; i<argc && !sig; i++){ //Spiega nella relazione
		xsem_wait(&sem_free_slots, QUI);
		buffer[messi % buf_size] = argv[i]; //Scrivo nel buffer
		messi += 1;
		xsem_post(&sem_data_items, QUI);
		sleep(delay); //Per mandare SIGINT
	}
	
	//Comunico ai thread che sono finiti i file
	
	xsem_wait(&sem_free_slots, QUI);
	buffer[messi % buf_size] = ":STOP:"; //terminazione
	messi += 1;
	xsem_post(&sem_data_items, QUI);
	
	for(int i=0; i<numt; i++){
		xpthread_join(t[i], NULL, QUI);
	}

	return 0;
}