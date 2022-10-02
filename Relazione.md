# BigFarmDoreFrancesco

###### RELAZIONE PROGETTO FINALE ANNO 2021/22

### Farm.c :
#### 1) Semafori: 
La scelta dei semafori è basata principalmente sulla familiarità nel loro utilizzo. I semafori però possono anche svolgere il ruolo di Condition Variable, e risultano essere più flessibili.

#### 2) Scrittura nella socket:
Quando si va a scrivere nella socket, la strategia attuata è quella di inviare il nome del file e la somma come stringhe nella socket.

#### 3) Gestione segnale SIGINT:
La prima strategia era quella di creare un thread gestore, che data una maschera, gestisse il segnale sigint come richiesto. Sorgeva il problema che se SIGINT non veniva inviato, il thread gestore non terminava e il programma non si arrestava. Si è risolto modificando la sigaction direttamente nel main. Per l'interruzione della scrittura nel buffer, si è utilizzata una variabile volatile, che viene modificata dalla funzione "gestore", l'handler della sigaction (Si noti il controllo del for alla riga 188).

### Client.c :

#### 1) Validità numeri della riga di comando:
Il controllo della validità è "complesso": poichè la "atol()" restituisce zero in caso di errore, la prima soluzione *"naive"* è quella di controllare se il return value della atol() sia diverso da zero. Ma se volessimo controllare se la somma in un file sia proprio zero, cadremmo in un errore. Per questo si è aggiunto un controllo aggiuntivo sulla **stringa** "0" (si veda *"strcmp(3)"* alla riga 85). L'implementazione scelta prevede che se il numero non è valido, non viene neanche inviato al collector, e viene visualizzato che il numero non è valido. Se il numero invece risulta ben scritto, viene visualizzato il nome del file associato o "Nessun File" nel caso non esista.


#### 2) Invio dei numeri al collector per il controllo:
La strategia è la medesima del punto 2 in **"Farm.c"**. Si noti che l'invio dei numeri sfrutta un array temporaneo per il salvataggio dei numeri validi, per separarli da quelli non validi (es. parole quali "casa" o "pippo", che non rappresentano quindi dei numeri): la complessità **in spazio** non è quindi costante asintoticamente.

#### 3) Dimensione dei buffer:
La gestione della dimensione dei buffer avviene allocando dinamicamente la stringa di dimensione "dim + 1" Perché aggiungiamo a mano il carattere '\0' per evitare che la fprintf dia errori di accesso alla stringa.

### Collector.py :
#### 1) Salvataggio coppie <nomeFile, somma>:
Il salvataggio avviene mediante un dizionario, che viene passato **per riferimento** ad ogni thread. In questo modo, viene modificata la copia originale del dizionario e le modifiche sono sempre visibili a tutti.

#### 2) Risposta a "Client.c":
Nel caso Client richieda coppie specifiche, per permettere la stampa ordinata delle coppie si fa utilizzo di una lista, che viene ordinata mediante la primitiva ".sort()" delle liste.


### In generale:
#### 1) Gestione dei numeri come int, long o stringhe:
I long (le somme) vengono mandate dalla farm e dal client sotto forma di stringhe. Una volta arrivati al collector però è stato necessario riconvertirli in numeri, per permettere l'ordinamento mediante la funzione "sorted", che altrimenti stamperebbe in ordine lessicografico (ad esempio "-1" risulta minore di "-12"). Il collector riceve la stringa e la "casta" a int per memorizzarla nel dizionario. Si noti che quando "Client" richiede una certa somma, viene fatto il cast anche del numero passato dal client,  perché altrimenti avremmo un confronto tra int e stringa, che produrrebbe sempre "Nessun File" come output.
