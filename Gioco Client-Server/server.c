//SERVER.C

//librerie
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
//colori
#define CYANO   "\x1B[1;36m"
#define YELLOW  "\x1B[1;33m"
#define RED     "\x1b[1;31m"
#define GREEN   "\x1b[1;32m"
#define RESET   "\x1b[0m"
#define BLU      "\x1B[1;34m"
//dichiarazione del tempo Totale
#define tempoTotale 120

//struttura dati dell'utente
struct Utente{
    char nickname[512];
    pid_t pid;
    int x;
    int y;
    struct Utente *next;
};

//puntatori al primo elemento delle strutture dati
struct Utente *Ut=NULL;

//funzioni Thread
void *menu(void *ptr);
void *creaMappa(void *ptr);

//funzioni generiche int(booleane)
int checkLogin(char *,int);
int controllaNome(char *,int);
int giaOnline(char * ,struct Utente *);

//funzioni generiche void
void error(char *);
void stampaLog(int ,char *);
void dataCorrente();
void getNickname(char *flag,char *);
void sendSocket(int ,char *,int );
void gioco(int conn,char *);
void stampaMappa(int fd,char Mappa[][20]);
void sig_handler(int);
void handlerpipe(int x);
void handlerCtrlC(int x);

//funzioni per la lista di utenti connessi
struct Utente *eliminaUtente(struct Utente *,char *);
void deallocaUtente(struct Utente *);
void aggiungiUtente(struct Utente **top,char *,pid_t);
void aggiungiPosizione(struct Utente *top,char *nickname,int i,int j);

//Variabili Globali
pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;
char Mappa[20][20];
int fdUtente;
int fdlog;
int utentiConnessi=0;
int MappaCreata=0;
int tempoGioco=0;
int nonvittoria=0;
int UtentiOnline=0;
int idGioco=0;


//inizio main
int main(int argc, char const *argv[]){
    
    int sock, Connesso;
    char datiRicevuti[1024];
    int bytesRicevuti=0;
    struct sockaddr_in server_addr,client_addr;
    int sockSize,i=0,j=0;
    
    fdUtente=open("utenti.txt", O_RDWR | O_CREAT |O_APPEND ,S_IRWXU);
    fdlog=open("log.txt",O_WRONLY | O_CREAT | O_APPEND,S_IRWXU);
    
    pthread_t tred;
    pthread_t threadServer;
    char data[256];
    
    signal(SIGINT,handlerCtrlC);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGSTOP,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGABRT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGPIPE,handlerpipe);
    
    if(fdUtente<0 || fdlog<0){
        error("Errore Permessi FILE\n");
    }
    
    if(argc!=2){
        error("Errore Inserire La Porta : Man D'Uso  ./server 5xxx\n");
    }
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error("Socket\n");
    }
    
    server_addr.sin_family = AF_INET; //accetta un formato host + porta
    server_addr.sin_port = htons(atoi(argv[1])); //porta
    server_addr.sin_addr.s_addr = INADDR_ANY; //indirizzo
 
    
    //bind assegna indirizzo di socket locale alla remota
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        error("Unable to bind\n");
    }
    
    sockSize = sizeof(struct sockaddr_in);
    
    //listen (socket , lunghezza coda di connessioni)
    if (listen(sock, 5) == -1) {
        error("Listen\n");
    }

    
    dataCorrente(data);
    bytesRicevuti=sprintf(datiRicevuti,"il server è up sulla %s port in Data: %s\n",argv[1],data);
    stampaLog(fdlog,datiRicevuti);
    signal(SIGPIPE,handlerpipe); 

    while(1){
        
        if((Connesso=accept(sock, (struct sockaddr *)&client_addr,&sockSize))==-1){
            error("Connessione rifiutata\n");
        }
        utentiConnessi++;
        
        if((pthread_create(&tred,NULL,menu,(void*)&Connesso)) < 0){
            error("pthread_create() Fallita\n");
        }
        pthread_detach(tred);
        sprintf(datiRicevuti,"Nuova Connessione Da : (%s,%d)\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        stampaLog(fdlog,datiRicevuti);
    }

    close(fdUtente);
    close(sock);
    close(fdlog);
    deallocaUtente(Ut);
    return 0;
}


void *menu(void *ptr){
    
    char Addnickname[512],messaggio[512],scelta[1024],data[256];
    int  choosed=1;
    int *connect=(int*)ptr;
    int conn=*connect;
    int nickname=0;
    int fdUtente_loc;
    struct Utente *tmp=NULL;
    int fdlog_loc, Online=0;
    int i=0;
    pthread_t threadServer;
    
    signal(SIGPIPE,handlerpipe);
    pthread_mutex_lock(&mymutex);//blocca il mutex
    fdUtente_loc=fdUtente;
    fdlog_loc=fdlog;
    pthread_mutex_unlock(&mymutex);//sblocca il mutex
    
    signal(SIGINT,handlerCtrlC);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGSTOP,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGABRT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGPIPE,handlerpipe);
    
    do{
        for(i=0; i<1024; i++){
            scelta[i]='\0';
        }
        if(nickname=read(conn,scelta,1024)<0){
            error("Errore read\n");
        }
        
        
        if(scelta[0]=='a'){
            
            scelta[nickname]='\0';
            
            if(controllaNome(scelta+1,fdUtente_loc)){
                stampaLog(fdUtente_loc,scelta+1);
                dataCorrente(data);
                sprintf(messaggio,"Nuovo utente registrato con %s in data : %s\n",scelta+1,data);
                stampaLog(fdlog_loc,messaggio);
                sendSocket(conn,"y",1);
            }else{
                dataCorrente(data);
                sprintf(messaggio,"Impossibile registrare l'Utente con %s in data : %s\n",scelta+1,data);
                stampaLog(fdlog_loc,messaggio);
                sendSocket(conn,"n",1);
            }
        }
        
        else if(scelta[0]=='b'){
            
            scelta[nickname]='\0';
            getNickname(Addnickname,scelta+1);
            pthread_mutex_lock(&mymutex);//blocca il mutex
            Online=giaOnline(Addnickname,Ut);
            struct Utente **tmp=&Ut;
            pthread_mutex_unlock(&mymutex);//sblocca il mutex
            
            if(Online==1){
                dataCorrente(data);
                sprintf(messaggio,"Impossibile effettuare il login l'Utente è gia loggato con %s in data : %s\n",scelta+1,data);
                stampaLog(fdlog_loc,messaggio);
                sendSocket(conn,"g",1);
                Online=0;
            }
            else{
                dataCorrente(data);
                sprintf(messaggio,"L'utente %s si e' connesso in data : %s\n",Addnickname,data);
                stampaLog(fdlog_loc,messaggio);
                if(!checkLogin(scelta+1,fdUtente_loc)){
                    sendSocket(conn,"y",1);
                    pthread_mutex_lock(&mymutex);//blocca il mutex
                    aggiungiUtente(tmp,Addnickname,pthread_self());
                    pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    if(MappaCreata==0){
                        if(pthread_create(&threadServer,NULL,creaMappa,NULL) < 0){
                            error("Errore Thread\n");
                        }
                    }
                    gioco(conn,Addnickname);
                }else{
                    sendSocket(conn,"n",1);
                }
                
            }
        }
        else if(scelta[0]=='c'){
            int fileLog_loc2=fdlog;
            dataCorrente(data);
            pthread_mutex_lock(&mymutex);//blocca il mutex
            char messaggio[256];
            sprintf(messaggio,"Utente non loggato disconnesso in Data %s\n",data);
            stampaLog(fileLog_loc2,messaggio);
            utentiConnessi--;
            pthread_mutex_unlock(&mymutex);//sblocca il mutex
            choosed=0;
        }else if(scelta[0]=='l'){
            pthread_mutex_lock(&mymutex);//blocca il mutex
            int fileLog_loc2=fdlog;
            dataCorrente(data);
            char messaggio[256];
            sprintf(messaggio,"Utente non loggato disconnesso in maniera anomala in Data %s\n",data);
            stampaLog(fileLog_loc2,messaggio);
            utentiConnessi--;
            pthread_mutex_unlock(&mymutex);//sblocca il mutex
            choosed=0;
            
        }else{
            pthread_mutex_lock(&mymutex);//blocca il mutex
            int fileLog_loc3=fdlog;
            dataCorrente(data);
            sprintf(messaggio,"Errore di comunicazione con client in Data %s\n",data);
            stampaLog(fileLog_loc3,messaggio);
            pthread_mutex_unlock(&mymutex);//sblocca il mutex
            choosed=0;
        }
    }while(choosed==1);
    
    pthread_exit(NULL);
}

void sendSocket(int conn,char *mess,int nbytes){
    if(write(conn,mess,nbytes)<0){
        error("Errore write\n");
    }
}



//_______________________________________________________________________________________

void error(char *messaggio){
    perror(messaggio);
    pthread_mutex_lock(&mymutex);//blocco il mutex in scrittura del file
    int fdlog_loc=fdlog;
    pthread_mutex_unlock(&mymutex);//sblocco il mutex in scrittura del file
    
    stampaLog(fdlog_loc,messaggio);
    exit(1);
}



//_______________________________________________________________________________________

void stampaLog(int fd,char *messaggio){
    if(write(fd,messaggio,strlen(messaggio))<0){
        error("Permessi Write\n");
    }
}



//_______________________________________________________________________________________

void dataCorrente(char *dataT)
{
    int gm,m,a,gs,h,min,s;
    time_t data;
    struct tm * leggibile = NULL;
    time (&data);
    
    leggibile = localtime (&data);
    gm=leggibile->tm_mday;
    m=leggibile->tm_mon +1;
    a=leggibile->tm_year+1900;
    gs=leggibile->tm_wday+1; // 1 = Domenica - 7 = Sabato
    h=leggibile->tm_hour;
    min=leggibile->tm_min;
    s=leggibile->tm_sec;
    sprintf(dataT,"%d/%d/%d Ora :(%d:%d:%d)",gm,m,a,h,min,s);
}


//_______________________________________________________________________________________

int controllaNome(char *nickname,int fd)
{
    char flag[1024], file[1024], nome[1024];
    int i=0,nbytes=0;
    off_t currpos;
    
    currpos = lseek(fd, 0, SEEK_SET);
    getNickname(flag,nickname);
    while(read(fd, &file[i], 1) == 1){
        if(file[i]=='\n'||file[i]==0x0){
            file[i]='\0';
            (nome,file);
            if(strcmp(flag,nome)==0){
                return 0;
            }
            i=0;
            continue;
        }
        i++;
    }
    return 1;
}


//_______________________________________________________________________________________

void getNickname(char *flag,char *nickname)
{
    int i=0;
    
    while(nickname[i]!=':'){
        flag[i]=nickname[i];
        i++;
    }
    flag[i]='\0';
}



//_______________________________________________________________________________________

int checkLogin(char *nickname, int fd)
{
    char flag[1024], file[1024], nome[1024];
    int i=0,nbytes=0;
    off_t currpos;
    
    currpos = lseek(fd, 0, SEEK_SET);
    while(read(fd, &file[i], 1) == 1){
        if(file[i]=='\n'||file[i]==0x0){
            file[i]='\n';
            file[i+1]='\0';
            if(strcmp(file,nickname)==0){
                return 0;
            }
            i=0;
            continue;
        }
        i++;
    }
    return 1;
}


//_______________________________________________________________________________________

void gioco(int conn, char *nickname)
{
    
    char coordinateClient[3], nuovaPosizione[3];
    srand(time(NULL));
    int i=0, j=0, flag=1, q=0,k=0, posizionato=0, Ric_messaggio=0, controlloId=0;
    char messaggio[1000];
    char lista[5000], data[256];
    int vittoria=0;
    pthread_t threadServer;
    int idGioco_client;
    pthread_mutex_lock(&mymutex);//blocco il mutex
    UtentiOnline++;
    pthread_mutex_unlock(&mymutex);//sblocco il mutex
    
    nuovaPosizione[0]='\0';
    nuovaPosizione[1]='\0';
    nuovaPosizione[2]='\0';
    
    do{
        
        i=rand()%19;
        j=0;
        if(Mappa[i][j]==' '){
            pthread_mutex_lock(&mymutex);//blocco il mutex
            Mappa[i][j]='U';
            pthread_mutex_unlock(&mymutex);//blocco il mutex
            posizionato=1;
        }
    }while(posizionato==0);
    coordinateClient[0]=i;
    coordinateClient[1]=j;
    coordinateClient[2]='U';
    
    //inviare qui l'id partita
    if(write(conn,&idGioco,sizeof(int))<0){
        error("Errore write\n");
    }
    //fine invio id partita
    
    sendSocket(conn,coordinateClient,3);
    signal(SIGINT,handlerCtrlC);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGSTOP,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGABRT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGPIPE,handlerpipe);
    do{
        
        pthread_mutex_lock(&mymutex);//blocco il mutex
        aggiungiPosizione(Ut,nickname,coordinateClient[0],coordinateClient[1]);
        pthread_mutex_unlock(&mymutex);//sblocco il mutex
        
        nuovaPosizione[0]=0;
        if(read(conn,nuovaPosizione,3)<0){
            flag=0;
        }
        
        else{
            
            switch(nuovaPosizione[0]){
                case 'w':
                    
                    if(read(conn,&idGioco_client,sizeof(int))<0){
                        error("Errore read");
                    }
                    if(tempoGioco==tempoTotale){
                        pthread_mutex_lock(&mymutex);//blocco il mutex
                        struct Utente *tmp=Ut;
                        pthread_mutex_unlock(&mymutex);//sblocco il mutex
                        
                        nonvittoria=1;
                        flag=0;
                    }
                    else if(idGioco!=idGioco_client){
                        sendSocket(conn,"f",1);
                        controlloId=1;
                        flag=0;
                    }else{
                        if((nuovaPosizione[1]-1)<0){
                            coordinateClient[0]=nuovaPosizione[1];
                            coordinateClient[1]=nuovaPosizione[2];
                        }
                        else{
                            if(Mappa[nuovaPosizione[1]-1][nuovaPosizione[2]]=='X'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='X';
                                pthread_mutex_lock(&mymutex);//blocca il mutex
                                Ut=eliminaUtente(Ut,nickname);
                                pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    		flag=0;
                    
                            }
                            else if(Mappa[nuovaPosizione[1]-1][nuovaPosizione[2]]=='U'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='U';
                            }
                            
                            else{
                                pthread_mutex_lock(&mymutex);//blocco il mutex
                                if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=='U'){
                                    Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=' ';
                                }
                                coordinateClient[0]=nuovaPosizione[1]-1;
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]=' ';
                                Mappa[nuovaPosizione[1]-1][nuovaPosizione[2]]='U';
                                pthread_mutex_unlock(&mymutex);//sblocco il mutex
                            }
                        }
                    }
                    if(nonvittoria==1){
                        sendSocket(conn,"f",1);
                        flag=0;
                    }else{
                        if(controlloId==0){
                            sendSocket(conn,coordinateClient,3);
                        
                        }
                    }
                    break;
                    
                case 's':
                    
                    if(read(conn,&idGioco_client,sizeof(int))<0){
                        error("Errore read");
                    }
                    if(idGioco!=idGioco_client){
                        sendSocket(conn,"f",1);
                        controlloId=1;
                        flag=0;
                    }
                    else if(tempoGioco==tempoTotale){
                        pthread_mutex_lock(&mymutex);//blocco il mutex
                        struct Utente *tmp=Ut;
                        pthread_mutex_unlock(&mymutex);//sblocco il mutex
                        nonvittoria=1;
                        flag=0;
                    }else{
                        if((nuovaPosizione[1]+1)>19){
                            coordinateClient[0]=nuovaPosizione[1];
                            coordinateClient[1]=nuovaPosizione[2];
                        }
                        else{
                            if(Mappa[nuovaPosizione[1]+1][nuovaPosizione[2]]=='X'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='X';
                                pthread_mutex_lock(&mymutex);//blocca il mutex
                                Ut=eliminaUtente(Ut,nickname);
                                pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    		flag=0;
                            }
                            else if(Mappa[nuovaPosizione[1]+1][nuovaPosizione[2]]=='U'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='U';
                            }
                            
                            
                            else{
                                pthread_mutex_lock(&mymutex);//blocco il mutex
                                if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=='U'){
                                    Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=' ';
                                }
                                coordinateClient[0]=nuovaPosizione[1]+1;
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]=' ';
                                Mappa[nuovaPosizione[1]+1][nuovaPosizione[2]]='U';
                                pthread_mutex_unlock(&mymutex);//sblocco il mutex
                            }
                        }
                    }
                    if(nonvittoria==1){
                        sendSocket(conn,"f",1);
                        flag=0;
                    }else{
                        if(controlloId==0){
                            sendSocket(conn,coordinateClient,3);
                           
                        }
                    }
                    break;
                    
                case 'a':
                    
                    if(read(conn,&idGioco_client,sizeof(int))<0){
                        error("Errore read");
                    }
                    if(idGioco!=idGioco_client){
                        sendSocket(conn,"f",1);
                        controlloId=1;
                        flag=0;
                    }
                    else if(tempoGioco==tempoTotale){
                        pthread_mutex_lock(&mymutex);//blocco il mutex
                        struct Utente *tmp=Ut;
                        pthread_mutex_unlock(&mymutex);//sblocco il mutex
                        nonvittoria=1;
                        flag=0;
                    }else{
                        if((nuovaPosizione[2]-1)<0){
                            coordinateClient[0]=nuovaPosizione[1];
                            coordinateClient[1]=nuovaPosizione[2];
                        }
                        else{
                            if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]-1]=='X'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='X';
                                pthread_mutex_lock(&mymutex);//blocca il mutex
                                Ut=eliminaUtente(Ut,nickname);
                                pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    		flag=0;
                            }
                            else if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]-1]=='U'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='U';
                            }
                            
                            
                            else{
                                pthread_mutex_lock(&mymutex);//blocco il mutex
                                if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=='U'){
                                    Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=' ';
                                }
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2]-1;
                                coordinateClient[2]=' ';
                                Mappa[nuovaPosizione[1]][nuovaPosizione[2]-1]='U';
                                pthread_mutex_unlock(&mymutex);//sblocco il mutex
                            }
                        }
                    }
                    if(nonvittoria==1){
                        sendSocket(conn,"f",1);
                        flag=0;
                    }else{
                        if(controlloId==0){
                            sendSocket(conn,coordinateClient,3);
                        
                        }
                    }
                    break;
                    
                case 'd':
                    
                    if(read(conn,&idGioco_client,sizeof(int))<0){
                        error("Errore read");
                    }
                    if(idGioco!=idGioco_client){
                        sendSocket(conn,"f",1);
                        controlloId=1;
                        flag=0;
                    }
                    else if(tempoGioco==tempoTotale){
                        pthread_mutex_lock(&mymutex);
                        struct Utente *tmp=Ut;
                        pthread_mutex_unlock(&mymutex);
                        
                        nonvittoria=1;
                        flag=0;
                    }else{
                        if((nuovaPosizione[2]+1)>19){
                            coordinateClient[0]=nuovaPosizione[1];
                            coordinateClient[1]=nuovaPosizione[2];
                        }
                        if(nuovaPosizione[2]==19){
                            vittoria=1;
                        }
                        else{
                            if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]+1]=='X'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='X';
                                pthread_mutex_lock(&mymutex);//blocca il mutex
                                Ut=eliminaUtente(Ut,nickname);
                                pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    		flag=0;
                            }
                            else if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]+1]=='U'){
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2];
                                coordinateClient[2]='U';
                            }
                            
                            else{
                                pthread_mutex_lock(&mymutex);//blocco il mutex
                                if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=='U'){
                                    Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=' ';
                                }
                                coordinateClient[0]=nuovaPosizione[1];
                                coordinateClient[1]=nuovaPosizione[2]+1;
                                coordinateClient[2]=' ';
                                Mappa[nuovaPosizione[1]][nuovaPosizione[2]+1]='U';
                                pthread_mutex_unlock(&mymutex);//sblocco il mutex
                            }
                        }
                    }
                    if(vittoria==1){
                        sendSocket(conn,"v",1);
                        pthread_mutex_lock(&mymutex);
                        deallocaUtente(Ut);
                        Ut=NULL;
                        flag=0;
                        pthread_mutex_unlock(&mymutex);
                    }else if(nonvittoria==1){
                        sendSocket(conn,"f",1);
                        flag=0;
                    }else{
                        if(controlloId==0){
                            sendSocket(conn,coordinateClient,3);
                         
                        }
                    }
                    break;
                    
                case 'c':
                    
                    dataCorrente(data);
                    pthread_mutex_lock(&mymutex);//blocca il mutex
                    int fileLog_loc=fdlog;
                    if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=='U'){
                        Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=' ';
                    }
                    Ut=eliminaUtente(Ut,nickname);
                    char messaggio[256];
                    sprintf(messaggio,"Utente %s Disconnesso in Data %s\n",nickname,data);
                    stampaLog(fileLog_loc,messaggio);
                    pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    flag=0;
                    break;
                    
                case 'i':
                    pthread_mutex_lock(&mymutex);//blocca il  mutex
                    struct Utente *tmp=Ut;
                    pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    for(k=0;k<5000;k++){
                        lista[k]='\0';
                    }
                    strcat(lista,GREEN"Utenti online:  "RESET);
			while(tmp!=NULL){
				char coordinateLoc[50];
				sprintf(coordinateLoc,"[%d][%d]",tmp->x,tmp->y);			
				strcat(lista,GREEN"\nUtente  "RESET);
				strcat(lista,tmp->nickname);
				strcat(lista,GREEN" in posizione  "RESET);
				strcat(lista,coordinateLoc);
				strcat(lista,"\n");
				tmp=tmp->next;
			}
                    sendSocket(conn,lista,strlen(lista));
                    break;
                    
                case 't':
                    pthread_mutex_lock(&mymutex);//blocca il  mutex
                    int tempo_loc=tempoGioco;
                    pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    if(write(conn,&tempo_loc,sizeof(int))<0){
                        error("Errore write\n");
                    }
                    break;
                    
                    
                case 'k':
                    pthread_mutex_lock(&mymutex);//blocca il  mutex
                    struct Utente *tmp2=Ut;
                    pthread_mutex_unlock(&mymutex);//sblocca il mutex
                    char lista2[5000];
                    for(k=0;k<5000;k++){
                        lista2[k]='\0';
                    }
                    while(tmp2!=NULL){
                        char coordinateLoc[50];
                        sprintf(coordinateLoc,"%d?%d",tmp2->x,tmp2->y);
                        strcat(lista2,coordinateLoc);
                        strcat(lista2,"&");
                        tmp2=tmp2->next;
                    }
                    lista2[strlen(lista2)-1]='\0';
                    sendSocket(conn,lista2,strlen(lista2));
                    break;
                    
                    
                default :
                    flag=0;
                    break;
            }
        }
    }while(flag==1);
    
    pthread_mutex_lock(&mymutex);
    if(Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=='U'){
        Mappa[nuovaPosizione[1]][nuovaPosizione[2]]=' ';
    }
    UtentiOnline--;
    pthread_mutex_unlock(&mymutex);
    
    Ut=eliminaUtente(Ut,nickname);
    
    if(vittoria==1){
        if(pthread_create(&threadServer,NULL,creaMappa,NULL) < 0){
            error("Errore Thread\n");
        }
    }
}



//_______________________________________________________________________________________

void stampaMappa(int fd,char Mappa[][20])
{
    int i=0, j=0,z=0;
    char messaggio[20]="Debug\0";
    write(fd,"---------------------\n",22);
    for(i=0; i<20; i++){
        write(fd,"|",1);
        for(j=0; j<20; j++){
            write(fd,&Mappa[i][j],1);
        }
        write(fd,"|\n",2);
    }
    write(fd,"---------------------\n",22);
}



//_______________________________________________________________________________________

void *creaMappa(void *ptr)
{
    int i=0, j=0, k=0;
    int  fdlog_loc;
    int posizionato=0;
    int cordinata1=0,cordinata2=0;
    char data[50], nuovaMappa[50];
    srand(time(NULL));
    time_t start=0,end=0;
    char messId[50];
    
    pthread_mutex_lock(&mymutex);
    idGioco++;//aumenta id partita
    sprintf(messId,"Inizia la partita numero ( %d )\n",idGioco);
    nonvittoria=0;
    fdlog_loc=fdlog;
    pthread_mutex_unlock(&mymutex);
    
    //inizializzazione della matrice
    for(i=0; i<20; i++){
        for(j=0; j<20; j++){
            Mappa[i][j]=' ';
        }
    }
    //posizonamento delle bombe
    for(i=0; i<8; i++){
        cordinata1=1+rand()%18;
        cordinata2=1+rand()%18;
        if(Mappa[cordinata1][cordinata2]==' '){
            Mappa[cordinata1][cordinata2]='X';
        }
    }
    MappaCreata=1;
    sprintf(nuovaMappa,"Nuova mappa generata in data %s\n",data);
    stampaLog(fdlog_loc,nuovaMappa);
    stampaMappa(fdlog_loc,Mappa);
    dataCorrente(data);
    tempoGioco=0;
    do{
        sleep(1);
        tempoGioco++;
    }while(tempoGioco<tempoTotale);
    
    MappaCreata=0;
    pthread_exit(NULL);
}


//_______________________________________________________________________________________

void aggiungiUtente(struct Utente **top,char *nickname,pid_t pidl){
    
    struct Utente *tmp=(struct Utente *)calloc(1,sizeof(struct Utente));
    strcpy(tmp->nickname,nickname);
    tmp->pid=pidl;
    tmp->next=(*top);
    (*top)=tmp;
}


//_______________________________________________________________________________________

struct Utente *eliminaUtente(struct Utente *top,char *nickname){
    if(top!=NULL){
        if(strcmp(top->nickname,nickname)==0){
            struct Utente *tmp=NULL;
            tmp=top;
            top=top->next;
            free(tmp);
        }else{
            top->next=eliminaUtente(top->next,nickname);
        }
    }
    return top;
}


//_______________________________________________________________________________________

int giaOnline(char *nickname ,struct Utente *top){
    int flag=0;
    if(top!=NULL){
        if(strcmp(top->nickname,nickname)==0){
            flag=1;
        }else{
            flag=giaOnline(nickname,top->next);
        }
    }
    return flag;
}



//_______________________________________________________________________________________

void deallocaUtente(struct Utente *top){
    if(top!=NULL){
        deallocaUtente(top->next);
        free(top);
    }
}



//_______________________________________________________________________________________

void aggiungiPosizione(struct Utente *top,char *nickname,int i,int j){
    if(top!=NULL){
        if(strcmp(top->nickname,nickname)==0){
            top->x=i;
            top->y=j;
        }else{
            aggiungiPosizione(top->next,nickname,i,j);
        }
    }
}


//_______________________________________________________________________________________

struct Utente *getNode(pid_t pid,struct Utente *top){
    struct Utente *flag=0;
    if(top!=NULL){
        if(top->pid==pid){
            flag=top;
        }else{
            flag=getNode(pid,top->next);
        }
    }
    return flag;
}


//_______________________________________________________________________________________

void handlerpipe(int x)
{
    void *status;
    char buffer[5000];
    struct Utente *Utente_local=NULL;
    time_t ora;
    pthread_mutex_lock(&mymutex);//blocco il mutex
    Utente_local=getNode(pthread_self(),Ut);
    if(Mappa[Utente_local->x][Utente_local->y]=='U'){
        Mappa[Utente_local->x][Utente_local->y]=' ';
    }
    pthread_mutex_unlock(&mymutex);
    if(Utente_local){
        ora=time(NULL);
        sprintf(buffer,"\t%s\tchiusura anomala\t%s\n",Utente_local->nickname,asctime(localtime(&ora)));
        pthread_mutex_lock(&mymutex);//sblocco il mutex
        Ut=eliminaUtente(Ut,Utente_local->nickname);
        pthread_mutex_unlock(&mymutex);
    }
    pthread_exit(status);
}


//_______________________________________________________________________________________
void handlerCtrlC(int x){
    void *status;
    pthread_mutex_lock(&mymutex);
    struct Utente *local=Ut;
    pthread_mutex_unlock(&mymutex);
    while(local!=NULL){
        pthread_exit(&local->pid);
        local=local->next;
    }
    pthread_exit(status);
}

