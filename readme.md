# Progetto Lab 2
# Gestione dei thread nella funzione `pagerank`


## Inizializzazione dei dati

All'inizio della funzione, vengono inizializzati diversi array e variabili necessari per il calcolo del PageRank. Questi includono:

-   `X_t`: un array che rappresenta i valori del PageRank all'iterazione `t`.
-   `Y`: un array utilizzato per calcolare un termine necessario del PageRank.
-   `X_t_1`: un array che conterrà i valori del PageRank per l'iterazione `t+1`.
-   `S`: un array che rappresenta un termine necessario del PageRank.
-   `errore`: una variabile che tiene traccia dell'errore durante il calcolo.

## Creazione del thread pool

Viene creato un pool di thread utilizzando la funzione `tp_create`, specificando il numero di thread da utilizzare.

## Calcolo del PageRank

Il calcolo del PageRank avviene all'interno di un ciclo `do-while`. Durante ogni iterazione di questo ciclo, vengono eseguite le seguenti operazioni:

1.  **Calcolo dei termini del PageRank**: I termini del PageRank vengono calcolati utilizzando diverse funzioni parallele che operano su dati condivisi. Queste funzioni vengono aggiunte al thread pool utilizzando `tp_add_work`.
    
2.  **Attivazione del thread pool**: Il thread principale attende il completamento di tutte le operazioni nel pool di thread utilizzando `tp_wait`.
    
3.  **Aggiornamento dei valori del PageRank**: Dopo il calcolo dei termini del PageRank, vengono aggiornati gli array `X_t` e `X_t_1`.
    
4.  **Verifica della condizione di terminazione**: Viene controllato se l'errore calcolato è inferiore alla soglia specificata `eps` e se il numero massimo di iterazioni `maxiter` non è stato superato. Se uno di questi criteri è soddisfatto, il ciclo termina.
    

## Segnalazione esterna

Durante ogni iterazione del ciclo, viene verificata la presenza di un segnale esterno `SIGUSR1`. Se tale segnale viene ricevuto, vengono stampati il numero di iterazioni finora eseguite, l'indice dell'elemento con il massimo PageRank e il valore del PageRank corrispondente.

## Pulizia delle risorse

Alla fine del calcolo del PageRank, vengono liberate tutte le risorse allocate, incluso il thread pool e gli array utilizzati per il calcolo.

----------

## Gestione dei thread nella struttura del thread pool

La struttura del thread pool, definita nel file `threadpool.h`, gestisce l'esecuzione dei thread per le operazioni parallele nella funzione `pagerank`. Ecco come è strutturata la gestione dei thread:

-   **Inizializzazione**: Il thread pool viene inizializzato utilizzando la funzione `tp_create`, che crea un numero specificato di thread e inizializza le condizioni e i mutex necessari.
    
-   **Aggiunta di lavoro**: La funzione `tp_add_work` aggiunge un nuovo lavoro al pool di thread. Questo lavoro consiste in una funzione (`func`) da eseguire e un argomento (`arg`) da passare alla funzione.
    
-   **Attesa del completamento**: La funzione `tp_wait` viene utilizzata per attendere il completamento di tutti i lavori nel pool di thread.
    
-   **Esecuzione del lavoro**: Ogni thread esegue la funzione `tp_worker`, che rimane in attesa di nuovi lavori da eseguire. Quando un lavoro è disponibile, viene prelevato dalla coda dei lavori e eseguito.
    
-   **Pulizia delle risorse**: Alla fine dell'esecuzione, il thread pool viene distrutto utilizzando la funzione `tp_destroy`, che libera tutte le risorse allocate.
    

Questo approccio consente una gestione efficiente e parallela delle operazioni nella funzione `pagerank`, migliorando le prestazioni complessive del calcolo del PageRank.

## Perché la scelta di un'implementazione di una libreria ThreadPool da 0?
Threadpool.c è stata preferita all'uso di una semplice pthread_create e pthread_join per via di un errore di lettura :) . 
Pensando di non poter utilizzare la pthread_join è stata usata la pthread_detach per la gestione dei thread. Questo ha portato all'implementazione di una coda di lavori molto più ordinata, avendo la possibilità di assicurarsi che i lavori siano finiti prima di iniziare altre operazioni di calcolo, e utilizzando sempre gli stessi thread, senza allocarne di nuovi.

## Work Flow nell'algoritmo
In maniera circolare fino al completamento dell'algoritmo, prima viene mandato in coda il calcolo di tutti i singoli termini di X(t+1), alla fine viene fatta una wait del completamento dei lavori.
Viene aggiornato X(t), ed in parallelo vengono aggiunti i calcoli della componente S, Y, e dell'errore.

Il thread dei segnali ha un handling a parte per semplicità, siccome la pthread_cancel usando una sigwait nella funzione handler era più comoda che maneggare con atomic flags globali, e anche perché il suo lavoro non viene completato per tutta la durata dell'algoritmo.

## Perché la sleep a fine main?
Per valgrind, siccome la sua esecuzione finisce prima che il S.O. descheduli tutti i thread e ne liberi la memoria. Quindi è per non avere errori di falsi positivi.


# Gestione dei thread nella parte Python Server
Il server Python è stato implementato per gestire le richieste dei client e calcolare il PageRank di grafi inviati dai client. Vediamo come i thread sono gestiti all'interno di questo server:

## Inizializzazione

Il server Python utilizza il modulo `socket` per creare un socket TCP che accetta le connessioni dei client. Viene inizializzata una lista `threads` per tracciare i thread attivi che gestiscono le connessioni dei client.

## Gestione del client

Per ogni connessione accettata, viene creato un nuovo thread tramite la funzione `threading.Thread`. Questo thread gestisce la connessione del client, riceve il grafo inviato dal client e calcola il PageRank.

## Gestione del segnale di interruzione

Il server gestisce il segnale di interruzione (`SIGINT`) per chiudere correttamente tutte le connessioni e terminare i thread in esecuzione prima di uscire. Questo viene fatto utilizzando la funzione `signal.signal` per impostare un gestore di segnali personalizzato.

## Funzione di gestione del client

La funzione `handle_client` legge il numero di nodi e archi inviati dal client, crea un file temporaneo e scrive il grafo nel formato richiesto. Successivamente, esegue il calcolo del PageRank utilizzando un'esecuzione di un processo subprocess di un'eseguibile `pagerank`. Infine, invia il risultato al client tramite il socket.

## Funzione di avvio del server

La funzione `start_server` crea un socket TCP, accetta le connessioni dei client in un ciclo continuo e avvia un nuovo thread per gestire ciascuna connessione.

## Pulizia delle risorse

Alla fine dell'esecuzione del server, tutte le risorse vengono pulite correttamente, inclusi i file temporanei e i thread in esecuzione.

# Gestione dei thread nella parte Python Client
Il client Python è stato progettato per inviare i dati dei grafi al server e ricevere i risultati del calcolo del PageRank. Vediamo come i thread sono gestiti all'interno di questo client:

## Inizializzazione

Il client Python legge i nomi dei file dai parametri della riga di comando e crea un thread separato per ciascun file. Viene inizializzata una lista `threads` per tracciare i thread attivi.

## Gestione dei file

Per ogni file specificato come argomento, viene creato un nuovo thread tramite la funzione `threading.Thread`. Questo thread gestisce il file, legge i dati dei grafi, li invia al server e riceve la risposta.

## Funzione di gestione dei file

La funzione `handle_file` legge i dati dal file specificato, stabilisce una connessione con il server utilizzando un socket TCP e invia i dati al server. Successivamente, riceve la risposta dal server e stampa il risultato ottenuto.

## Esecuzione dei thread

Dopo la creazione di tutti i thread, il client attende il completamento di ciascun thread utilizzando la funzione `join`.

## Pulizia delle risorse

Alla fine dell'esecuzione, tutte le risorse vengono pulite correttamente, inclusi i thread in esecuzione.
