#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#define COMMAND_MAX_LEN 80
#define ALLUNGAMENO_TESTO 20
#define RIGA_MAX_LEN 1025
#define BACKUP_INTERVAL 30


typedef struct Comando{
    char tipo;
    int ind1;
    int ind2;
    int numero;
    char** testoSalvato;
    struct Comando* prev;
    struct Comando* next;
    char** backup;
    long int lunghezzaBackup;
}Comando;

Comando* cronoComandi;
Comando* comandoCorrente;
Comando** indiceComandi;

int opTot = 0;          // Numero totale di comandi dati
int opCorrente = 0;     // Indice dell'ultima operazione effettivamente eseguita
int opEffettiva = 0;    // Indice dell'operazione da eseguire una volta considerati tutti gli UNDO e REDO

int contatore = 0;

char** testo;
int lunghezzaTesto = 0;

void decodificaComando(char* stringa, Comando* comando);
void aggiungiComandoCronologia(Comando* comando);
void preEsecuzione(Comando* comando);
void salvaBackup(Comando* comando);
int caricaBackup(int posizione);
void eseguiComando(Comando* comando);
void postEsecuzione(Comando* comando);
void allungaTesto(int allungamento);
void leggiTestoChange(Comando* comando);
void change(Comando* comando);
void delete(Comando* comando);
void print(Comando* comando);
void undo(int n);
void redo(int n);
void fakeUndo(Comando* comando);
void fakeRedo(Comando* comando);
void eseguiUndoRedo();
void eseguiOpposto(Comando* comando);

int main() {
    char comandoStringa[COMMAND_MAX_LEN];
    Comando* comando = NULL;

    do {
        comando = malloc(sizeof(Comando));
        fgets(comandoStringa, COMMAND_MAX_LEN, stdin);

        decodificaComando(comandoStringa, comando);

        //printf("Tipo: %c\nInd1: %d\nInd2: %d\nNumero: %d\n", comando.tipo, comando.ind1, comando.ind2, comando.numero);
        preEsecuzione(comando);

        eseguiComando(comando);

        postEsecuzione(comando);

    }while (comando->tipo != 'q');

    return 0;
}

void decodificaComando(char* stringa, Comando* comando){
    const char delimitatore = ',';
    char *token = NULL;

    comando->tipo = stringa[strlen(stringa) - 2];

    switch(comando->tipo){
        case 'c':
            token = strtok(stringa, &delimitatore);
            comando->ind1 = atoi(token);
            token = strtok(NULL, &delimitatore);
            comando->ind2 = atoi(token);

            if(comando->ind1 == 0)
                comando->ind1 = 1;

            comando->testoSalvato = malloc((comando->ind2 - comando->ind1 + 1) * sizeof(char*));
            break;

        case 'd':
            token = strtok(stringa, &delimitatore);
            comando->ind1 = atoi(token);
            token = strtok(NULL, &delimitatore);
            comando->ind2 = atoi(token);

            if(comando->ind1 == 0)
                comando->ind1 = 1;

            break;

        case 'p':
            token = strtok(stringa, &delimitatore);
            comando->ind1 = atoi(token);
            token = strtok(NULL, &delimitatore);
            comando->ind2 = atoi(token);

            break;

        case 'u':
        case 'r':
            comando->numero = atoi(stringa);
            break;

        default:
            break;
    }
}

void aggiungiComandoCronologia(Comando* comando){
    if(opCorrente == 0){
        comando->prev = NULL;
        cronoComandi = comando;
        comandoCorrente = cronoComandi;
        opCorrente++;
        opTot = opCorrente;
        opEffettiva = opCorrente;

    }
    else{
        comandoCorrente->next = comando;
        comandoCorrente->next->prev = comandoCorrente;
        comandoCorrente = comandoCorrente->next;

        opCorrente++;
        opTot = opCorrente;
        opEffettiva = opCorrente;
    }

    //todo considerare che se si crea un nuovo ramo della lista, opTot va modificato
    //todo fare caso del primo comano inserito
}


void allungaTesto(int allungamento) {
    lunghezzaTesto += allungamento;
    testo = realloc(testo, lunghezzaTesto * sizeof(char*));

    for (int i = lunghezzaTesto - allungamento; i < lunghezzaTesto; i++) {
        testo[i] = malloc(3 * sizeof(char));
        testo[i][0] = '.';
        testo[i][1] = '\n';
        testo[i][2] = '\0';
    }
}

void preEsecuzione(Comando* comando){
    switch (comando->tipo) {
        case 'c':
            eseguiUndoRedo();
            leggiTestoChange(comando);
            aggiungiComandoCronologia(comando);
            break;

        case 'd':
            eseguiUndoRedo();
            aggiungiComandoCronologia(comando);
            break;

        case 'p':
            eseguiUndoRedo();
            break;

        default:
            break;
    }
}

void salvaBackup(Comando* comando){
    if((opTot % BACKUP_INTERVAL) != 0)
        return;

    comando->backup = malloc(lunghezzaTesto * sizeof(char*));

    for(int i = 0; i < lunghezzaTesto; i++)
        comando->backup[i] = testo[i];

    comando->lunghezzaBackup = lunghezzaTesto;

    indiceComandi = realloc(indiceComandi, (opTot / BACKUP_INTERVAL) * sizeof(Comando*));
    indiceComandi[(opTot / BACKUP_INTERVAL) - 1] = comando;

    //printf("\nSALVATAGGIO BACKUP %d %s\n", opTot,indiceComandi[(opTot / BACKUP_INTERVAL) - 1]->backup[0]);
}

int caricaBackup(int posizione){
    int posizioneBackup = (posizione / BACKUP_INTERVAL);


    if(posizioneBackup == 0){
        opCorrente = posizioneBackup;
        testo = NULL;
        lunghezzaTesto = 0;
        allungaTesto(ALLUNGAMENO_TESTO);
        return posizioneBackup;
    }

    comandoCorrente = indiceComandi[posizioneBackup - 1];

    posizioneBackup = posizioneBackup * BACKUP_INTERVAL;

    opCorrente = posizioneBackup;
    //printf("\nCARICAMENTO BACKUP POS %d\n", posizione);
    lunghezzaTesto = comandoCorrente->lunghezzaBackup;

    testo = malloc(lunghezzaTesto * sizeof(char*));
    for(int i = 0; i < comandoCorrente->lunghezzaBackup; i++)
        testo[i] = comandoCorrente->backup[i];



    return posizioneBackup;
}

void eseguiComando(Comando* comando) {

    switch(comando->tipo){
        case 'c':		//Change
            while(comando->ind2 + 1 > lunghezzaTesto)
                allungaTesto(ALLUNGAMENO_TESTO);
            change(comando);
            break;

        case 'd':		//Delete
            while(comando->ind2 + 1 > lunghezzaTesto)
                allungaTesto(ALLUNGAMENO_TESTO);
            delete(comando);
            break;

        case 'p':		//Print
            while(comando->ind2 + 1 > lunghezzaTesto)
                allungaTesto(ALLUNGAMENO_TESTO);
            print(comando);
            break;

        case 'u':		//Undo
            fakeUndo(comando);
            break;

        case 'r':		//Redo
            fakeRedo(comando);
            break;

        case 't':
            printf("\nopTot   %d\n", opTot);
            printf("opCorrente   %d\n", opCorrente);
            printf("opEffettiva   %d\n\n", opEffettiva);
            break;

        default:
            break;
    }
}

void postEsecuzione(Comando* comando){
    switch (comando->tipo) {
        case 'c':
        case 'd':
            salvaBackup(comando);
            break;

        default:
            break;
    }
}

void change(Comando* comando){
    //todo spostare leggitesto

    int inizio = comando->ind1;
    int fine = comando->ind2;
    char* temp;

    for(int i = inizio - 1 ; i < fine; i++){

        testo[i] = comando->testoSalvato[i - inizio + 1];

    }
}

void leggiTestoChange(Comando* comando){
    char buffer[RIGA_MAX_LEN];
    int lunghezza = comando->ind2 - comando->ind1 + 1;
    for(int i = 0; i < lunghezza; i++){
        fgets(buffer, RIGA_MAX_LEN, stdin);
        contatore++;
        comando->testoSalvato[i] = malloc((strlen(buffer) + 1) * sizeof(char));
        memcpy(comando->testoSalvato[i], buffer, strlen(buffer)+1);
    }
    char rigaInutile[3];
    fgets(rigaInutile, 3, stdin);
    contatore++;
}

void delete(Comando* comando){
    int inizio = comando->ind1;
    int fine = comando->ind2;
    int lunghezza = fine - inizio + 1;



    for(int i = inizio - 1 ; i < lunghezzaTesto - lunghezza; i++)
        testo[i] = testo[i + lunghezza];

    lunghezzaTesto -= lunghezza;


    //todo fare indice che tiene conto della lunghezza del testo effettiante scritto e fare il copia fino a quel punto
}

void insert(Comando* comando){
    int inizio = comando->ind1 - 1;
    int fine = comando->ind2 - 1;
    int lunghezza = fine - inizio + 1;

    allungaTesto(lunghezza);

    for(int i = lunghezzaTesto - 1; i > fine; i--)
        testo[i] = testo[i - lunghezza];

    for(int i = inizio; i < fine + 1; i++)
        testo[i] = comando->testoSalvato[i - inizio];

}

void print(Comando* comando){
    int inizio = comando->ind1;
    int fine = comando->ind2;

    if(fine <= 0)
        puts(".");
    else{
        if(inizio == 0)
            inizio++;

        for (int i = inizio - 1; i < fine; i++) {
            fputs(testo[i], stdout);
        }
    }
}

void undo(int n){

    for(int i = 0; i < n; i++){
        eseguiOpposto(comandoCorrente);
        comandoCorrente = comandoCorrente->prev;
    }
    opCorrente = opEffettiva;
}

void redo(int n){
    int i = 0;
    if(opCorrente == 0 && n > 0){
        comandoCorrente = cronoComandi;
        eseguiComando(comandoCorrente);
        i++;
    }
    for(; i < n; i++){

        comandoCorrente = comandoCorrente->next;
        eseguiComando(comandoCorrente);

    }
    opCorrente = opEffettiva;
}

void fakeUndo(Comando* comando){
    opEffettiva -= comando->numero;
    if(opEffettiva < 0)
        opEffettiva = 0;
}

void fakeRedo(Comando* comando){
    opEffettiva += comando->numero;
    if(opEffettiva > opTot)
        opEffettiva = opTot;
}

void eseguiUndoRedo(){
    //("chiamata eseguiUndoRedo\n");
    int n = opCorrente - opEffettiva;
    if(n == 0)
        return;
/*
    if(abs(n) < BACKUP_INTERVAL){
        if(n > 0)
            undo(n);
        else if(n < 0)
            redo(0 - n);
    }*/

    int posizioneBackup = caricaBackup(opEffettiva);

    redo(opEffettiva - posizioneBackup);


}

void eseguiOpposto(Comando* comando){

    switch ((comando->tipo)) {
        case 'c':
            change(comando);
            break;

        case 'd':
            insert(comando);
            break;

        default:
            break;
    }
}