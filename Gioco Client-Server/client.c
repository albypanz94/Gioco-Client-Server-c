//client.c//

//librerie
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
//Colori
#define CYANO   "\x1B[1;36m"
#define YELLOW  "\x1B[1;33m"
#define RED     "\x1b[1;31m"
#define GREEN   "\x1b[1;32m"
#define RESET   "\x1b[0m"
#define BLU      "\x1B[1;34m"
#define tempoTot 120


//funzioni

void stampa(char *messaggio);
//stampa a video il frontend del menu
void menu();
//pulisce il buffer
void pulisciBuffer();
//esegue una perror sul messaggio
void error(char *messaggio);
//stampa la mappa di gioco con giocatori
void stampaMappa(char A[][20],int,int,int,char *);
//funzione principale del gioco
void gioco(int sock);
//invia una socket al server
void sendSocket(int conn,char *mess,int nbytes);
//funzioni sui segnali
void sighandler(int segnale_ricevuto);
void sighandler1(int segnale_ricevuto);
void sighandler2(int segnale_ricevuto);
void sighandler3(int segnale_ricevuto);
void sighandler4(int segnale_ricevuto);
//stampa a video la vittoria in caso di raggiungimento traguardo
void stampaVittoria();
//stampa a video la sconfitta in caso di bomba
void stampaSconfitta();
//pausa
void pauseT();



//variabili globali

//variabile che determina
int giocato=0;
//variabile che stabilisce se sono in partita
int inGioco=0;
int connessioneFlag=0;
int flagPosizione1=0;
int flagPosizione2=0;


int main(int argc, char const *argv[])
{
    int uscita=0; //variabile per il logout
    int sock; //fd della socket
    int i=0, j=0;
    int bytesnickname=0, bytepassword=0;
    char Login[1024];
    char risposta;
    char scelta; //scelta di selezione
    char Nickname[256], Password[256], Log[512];
    struct hostent *host;
    struct sockaddr_in server_addr;
    
    
    //controllo input all'avvio del client
    if(argc!=3){
        error(RED"Error : Man = ./client ip(xxx.xxx.xxx) port(5XXX)\n"RESET);
    }
    
    host = gethostbyname(argv[1]);
    
    if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
        error(RED"ERROR : Socket\n"RESET);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);
    
    if(connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        error(RED"ERROR : Connect\n"RESET);
    }
    
    connessioneFlag=sock;
    system("clear");
    
    //inizio frontend menu registrazione,login,logout
    do{
        if(giocato==1){
            sleep(1);
            sprintf(Login,"b%s:%s\n",Nickname,Password);
            
            if(write(sock,Login,strlen(Login))<0){
                error("Errore WRITE\n");
            }
            
            if(read(sock,&risposta,1)<0){
                error("Error Read\n");
            }
            inGioco=1;
            gioco(sock);
        }
        
        
        else{
            //scelta tasti di input
            scelta=0;
            menu();
            
            
            
            signal(SIGINT,sighandler1);
            signal(SIGQUIT,sighandler1);
            signal(SIGHUP,sighandler2);
            signal(SIGSTOP,sighandler2);
            signal(SIGTERM,sighandler2);
            signal(SIGABRT,sighandler2);
            signal(SIGTSTP,sighandler2);
            signal(SIGPIPE,sighandler4);
            
            if(read(1,&scelta,1)<0){
                error("error write\n");
            }
            
            //a) registrazione utente
            if(scelta=='a'){
                pulisciBuffer();
                
                stampa(BLU"Inserire Nickname e Password\n"RESET);
                stampa("Nickname : ");
                
                if((bytesnickname=read(1,Nickname,256))<0){
                    error("Errore write\n");
                }
                Nickname[bytesnickname-1]='\0';
                
                stampa("\nPassword : ");
                if((bytepassword=read(1,Password,256))<0){
                    error("Errore write\n");
                }
                Password[bytepassword-1]='\0';
                
                sprintf(Login,"a%s:%s\n",Nickname,Password);
                
                if(write(sock,Login,strlen(Login))<0){
                    error("Errore WRITE\n");
                }
                
                if(read(sock,&risposta,1)<0){
                    error("Error Read\n");
                }
                switch(risposta){
                    case 'y': stampa(GREEN"Registrazione avvenuta con Successo, verrai indirizzato alla schermata iniziale\n"RESET);pauseT(); system("clear");break;
                    case 'n': stampa(RED"Il Nickname è gia stato usato, verrai indirizzato alla schermata iniziale\n"RESET); pauseT(); system("clear"); break;
                    default : error("Impossibile comunicare con il server\n"); break;
                }
            }
            
            //b) login
            else if(scelta=='b'){
                pulisciBuffer();
                
                stampa(BLU"Inserire Nickname e Password Per Il Login\n"RESET);
                stampa("Nickname : ");
                
                if((bytesnickname=read(1,Nickname,256))<0){
                    error("Errore write\n");
                }
                Nickname[bytesnickname-1]='\0';

                stampa("\nPassword : ");
                if((bytepassword=read(1,Password,256))<0){
                    error("Errore write\n");
                }
                Password[bytepassword-1]='\0';
                
                sprintf(Login,"b%s:%s\n",Nickname,Password);
                if(write(sock,Login,strlen(Login))<0){
                    error("Errore WRITE\n");
                }
                if(read(sock,&risposta,1)<0){
                    error("Error Read\n");
                }
                switch(risposta){
                    case 'y':system("clear"); stampa(GREEN"Adesso puoi iniziare a giocare!\n"RESET);
                        inGioco=1;
                        gioco(sock);
                        break;
                        
                    case 'n': stampa(RED"Login fallito utente non registrato\n"RESET); pauseT(); system("clear");
                        break;
                        
                    case 'g': stampa(RED"Login fallito utente già Online\n"RESET); pauseT(); system("clear");
                        break;
                        
                    default : stampa("Impossibile comunicare con il server\n");
                        break;
                }
            }

            //c) logout
            else if(scelta=='c'){
                stampa(YELLOW"Chiusura Del Client In Corso\n"RESET);
                sleep(1);
                
                if(write(sock,"c",1)<0){//comunica al server che è stata inserita una c
                    error("error write\n");
                }
                uscita=1;
            }
            else{
                system("clear");
                pulisciBuffer();
                stampa(RED"Error input riprova\n"RESET);
            }
        }
    }while(!uscita);
    
    close(sock);
    pulisciBuffer();
    return 0;
}


//______________________________________________________________________________FUNZIONE
void menu(){
    system("clear");
    stampa(GREEN);
    stampa("  PROGETTO ESAME Lab Sistemi Operativi\nDocente: Alberto Finzi\nAnno_ 2018/19"RESET);
    stampa(BLU"\nAlberto Panzera N86002772\nPellecchia Chiara N86002277\n\n"RESET);
    stampa("________________________\n");
    stampa("      MENU DI GIOCO     \n");
    stampa("------------------------\n");
    stampa("a) Crea un Account      \n");
    stampa("b) Login                \n");
    stampa("c) Logout               \n");
    stampa("------------------------\n");
}

//______________________________________________________________________________FUNZIONE
void stampa(char *messaggio){
    int l=strlen(messaggio);
    if(write(0,messaggio,l)<0){
        error("Error write\n");
    }
}

//______________________________________________________________________________FUNZIONE
void pulisciBuffer(){
    char ch;
    while((ch=getchar())!='\n');
}

//______________________________________________________________________________FUNZIONE
void error(char *messaggio){
    perror(messaggio);
    exit(1);
}

//______________________________________________________________________________FUNZIONE
void gioco(int sock){
    
    int i=0, j=0;
    int k=0, vittoria=0;
    char nuovaPosizione[3], miaPosizone[3], y=0;
    char Mappa[20][20], lista[5000], prova;
    char messaggio[256];
    int id_corrente, id_server;
    int tempo=0;
    char tempo_loc[256];
    
    //posizina "-" per ogni posizione della mappa
    for(i=0;i<20;i++){
        for(j=0; j<20; j++){
            Mappa[i][j]='-';
        }
    }
    
    //leggere qui il primo id
    if(read(sock,&id_corrente,sizeof(int))<0){
        error("errore read\n");
    }
    
    //fine lettura primo id
    if(read(sock,nuovaPosizione,3)<0){
        error("errore read\n");
    }
    
    i=nuovaPosizione[0];
    j=nuovaPosizione[1];
    Mappa[i][j]=nuovaPosizione[2];
    system("clear");
    stampaMappa(Mappa,i,j,id_corrente,NULL);
    
    
    do{
        signal(SIGINT,sighandler);
        signal(SIGQUIT,sighandler);
        signal(SIGHUP,sighandler3);
        signal(SIGSTOP,sighandler3);
        signal(SIGTERM,sighandler3);
        signal(SIGTSTP,sighandler3);
        signal(SIGPIPE,sighandler4);
        signal(SIGABRT,sighandler3);
        
        flagPosizione1=i;
        flagPosizione2=j;
        
        
        if(read(1,&miaPosizone[0],1)<=0){
            error("error read\n");
        }
        pulisciBuffer();
        
        int z=0;
        for(z=0; z<256; z++){
            messaggio[z]='\0';
        }
        
        //risposta in base alla posizione
        switch(miaPosizone[0]){
                
            //w) spostamento in alto
            case 'w':
                miaPosizone[1]=i;
                miaPosizone[2]=j;
                sendSocket(sock,miaPosizone,3);
                
                //write id del client verso il server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                //legge la risposta del server
                if(read(sock,nuovaPosizione,3)<0){
                    error(RED"Error Read\n"RESET);
                    inGioco=0; giocato=0;
                }
                
                //Tempo scaduto o vince un altro
                if(nuovaPosizione[0]=='f'){
                    system("clear");
                    stampa(RED"Partita terminata!\n"RESET);
                    inGioco=0;
                    giocato=1;
                }
                
                    if(nuovaPosizione[2]=='X'){
                        stampaSconfitta();
                        sleep(1);
                        inGioco=0;
                        giocato = 0;
                    }
                
                    else{
                        Mappa[i][j]='-';
                        i=nuovaPosizione[0]; j=nuovaPosizione[1];
                        //Mappa[i][j]='U';
                        
                        
                        for(k=0;k<5000;k++){
                            lista[k]='\0';
                        }
                        
                        sendSocket(sock,"k",1);
                        if(read(sock,lista,5000)<0){
                            error("errore read\n");
                        }
                        stampaMappa(Mappa,i,j,id_corrente,lista);
                    }
                break;
                
            //s) spostamento in basso
            case 's':
                miaPosizone[1]=i;
                miaPosizone[2]=j;
                sendSocket(sock,miaPosizone,3);
                
                //invio dell id al server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,nuovaPosizione,3)<0){
                    error(RED"Error Read\n"RESET);
                    inGioco=0; giocato=0;
                }
                
                if(nuovaPosizione[0]=='f'){
                    system("clear");
                    stampa(RED"Partita terminata!\n"RESET);
                    inGioco=0;
                    giocato=1;
                }
        
                    if(nuovaPosizione[2]=='X'){
                        stampaSconfitta();
                        sleep(1);
                        inGioco=0;
                        giocato=0;
                    }
        
                    else{
                        Mappa[i][j]='-';
                        i=nuovaPosizione[0]; j=nuovaPosizione[1];
                        //Mappa[i][j]='U';
                        
                        for(k=0;k<5000;k++){
                            lista[k]='\0';
                        }
                        
                        sendSocket(sock,"k",1);
                        if(read(sock,lista,5000)<0){
                            error("errore read\n");
                        }
                        stampaMappa(Mappa,i,j,id_corrente,lista);
                    }
                break;
                
            //a) spostamento a sinistra
            case 'a':
                miaPosizone[1]=i;
                miaPosizone[2]=j;
                sendSocket(sock,miaPosizone,3);
                
                //invio dell id al server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,nuovaPosizione,3)<0){
                    error(RED"Error Read\n"RESET);
                    inGioco=0; giocato=0;
                }
                
                if(nuovaPosizione[0]=='f'){
                    system("clear");
                    stampa(RED"Partita terminata!\n"RESET);
                    inGioco=0;
                    giocato=1;
                }
        
                    if(nuovaPosizione[2]=='X'){
                        stampaSconfitta();
                        sleep(1);
                        inGioco=0;
                        giocato=0;
                    }
        
                    else{
                        Mappa[i][j]='-';
                        i=nuovaPosizione[0]; j=nuovaPosizione[1];
                        //Mappa[i][j]='U';
                        
                        for(k=0;k<5000;k++){
                            lista[k]='\0';
                        }
                        
                        sendSocket(sock,"k",1);
                        if(read(sock,lista,5000)<0){
                            error("errore read\n");
                        }
                        stampaMappa(Mappa,i,j,id_corrente,lista);
                    }
                break;
            
                
            //d) spostamento a destra
            case 'd':
        
                miaPosizone[1]=i;
                miaPosizone[2]=j;
                sendSocket(sock,miaPosizone,3);
                
                //invio dell id al server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,nuovaPosizione,3)<0){
                    error(RED"Error Read\n"RESET);
                    inGioco=0; giocato=0;
                }
                
                if(nuovaPosizione[0]=='v'){
                    stampaVittoria();
                    sleep(1);
                    inGioco=0;
                    vittoria=1;
                    giocato=1;
                }else if(nuovaPosizione[0]=='f'){
                    system("clear");
                    stampa(RED"Partita terminata!\n"RESET);
                    inGioco=0;
                    giocato=1;
                }
                
                    else if(nuovaPosizione[2]=='X'){
                        stampaSconfitta();
                        sleep(1);
                        inGioco=0;
                        giocato=0;
                    }
                    else{
                        Mappa[i][j]='-';
                        i=nuovaPosizione[0]; j=nuovaPosizione[1];
                        //Mappa[i][j]='U';
                        
                        for(k=0;k<5000;k++){
                            lista[k]='\0';
                        }
                        sendSocket(sock,"k",1);
                        if(read(sock,lista,5000)<0){
                            error("errore read\n");
                        }
                        stampaMappa(Mappa,i,j,id_corrente,lista);
                    }
                break;
                
                
            //c) logout
            case 'c':
                
                stampa(YELLOW"Stai per uscire dal gioco e tornare al menu iniziale\n"RESET);
                miaPosizone[1]=i;
                miaPosizone[2]=j;
                sendSocket(sock,miaPosizone,3);
                sleep(1); inGioco=0; giocato=0;
                break;
                
            //i) info utenti
            case 'i': 
                for(k=0;k<5000;k++){
                    lista[k]='\0';
                }
                sendSocket(sock,"i",1);
                if(read(sock,lista,5000)<0){
                    error("error read\n");
                }
                stampa(lista);
                pauseT();
		
                for(k=0;k<5000;k++){
        		lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        error("errore read lista coordinate\n");
                    }
        		stampaMappa(Mappa,i,j,id_corrente,lista);
                break;
                
                
            //t) info tempo
            case 't': 
                
                sendSocket(sock,"t",1);
                if(read(sock,&tempo,sizeof(int))<0){
                    error("error read\n");
                }
                int tempoRimanente=tempoTot-tempo;
                sprintf(tempo_loc,GREEN"Tempo Trascorso :%d(sec)"RESET RED"\n\nTempo Rimanente :%d(sec)\n"RESET,tempo,tempoRimanente);
                stampa(tempo_loc);
                pauseT();

                for(k=0;k<5000;k++){
        			lista[k]='\0';
                }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        error("errore read lista coordinate\n");
                    }
        		stampaMappa(Mappa,i,j,id_corrente,lista);
                break;
                
            //caso default
            default :
                stampa(RED"Inserimento Non Valido\n"RESET); pulisciBuffer(); break;
        
        }
    }while(inGioco);
    
    sleep(1);
    pauseT();
}


//______________________________________________________________________________FUNZIONE

void stampaMappa(char Mappa[][20],int pos1, int pos2,int id_corrente,char *otherplayers){
    
    int i=0, j=0;
    char messaggio[256];
    char flag[256];
    
    //pausaGioco();
    for(i=0;i<20;i++){
        for(j=0; j<20; j++){
            if(i==pos1 && j==pos2);
            else
                Mappa[i][j]='-';
        }
    }
    
    //analisi stringa nemici
    int in;
    int temp=0;
    int nem[40];
    int indNem=0;
    
    if(otherplayers != NULL){
        
        //scorro la stringa
        for(in=0 ; in<strlen(otherplayers); in++){
            
            //se e un numero
            if(otherplayers[in]!='?' && otherplayers[in]!='&' && otherplayers[in]!='\0' ){
                temp = 0; //svuoto temp
                temp = (int)otherplayers[in]-'0'; // lo salvo
                
                // solo se dopo c'è un altro numero faccio questo
                if(otherplayers[in+1]!='?' && otherplayers[in+1]!='&' && otherplayers[in+1]!='\0'){
                    temp = 0; //svuoto temp
                    temp = (int)otherplayers[in+1]-'0'; // lo salvo
                    temp = temp+10; // lo incremento di 10
                    in++; //in questo caso avanzo di uno (me ne fotto del primo)
                }
                //a questo punto in temp ho il numero giuso (1 cifra o 2 cifre)
                //dopo [in] c'è sicuro un carattere
        
                if(otherplayers[in+1]=='?' || otherplayers[in+1]=='&' || otherplayers[in+1]=='\0'){
                    nem[indNem]=temp;
                    indNem++;
                }
            }
        } //fine for
        
        
        int indNem2; //indice array nemico (per scorrere)
        int mappaX;
        int mappaY;
        
        //a questo punto ho un array di interi dove ogni 2 caselle sono [x][y][x][y]...
        
        for (indNem2=0;indNem2<indNem;indNem2++){
            mappaX= nem[indNem2];
            indNem2++;
            mappaY= nem[indNem2];
            //segno il nemico in posizione giusta
            Mappa[mappaX][mappaY] = 'U';
        }
    } //fine if !=NULL
    //fine posizionamento nemici
    
    write(0,"\n",sizeof("\n"));
    write(0,"\t    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 \n",sizeof("\t    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 \n"));
    
    for(i=0; i<20; i++){
        if(i<10){
            sprintf(flag,"\t %d[ ",i);
        }else{
            sprintf(flag,"\t%d[ ",i);
        }
        stampa(flag);
        
        for(j=0; j<20; j++){
            if(Mappa[i][j]=='U'  && (i==pos1) && (j==pos2)){
                stampa(BLU"@"RESET);
            }
            else if(Mappa[i][j]=='U'  && (i!=pos1) && (j!=pos2)){
                stampa("@");
            }
            else{
                write(0,&Mappa[i][j],1);
            }
            if(j==19){
                write(0," ",sizeof(" "));
            }else{
                write(0,"  ",sizeof("  "));
            }
        }
        if(i==13){
            write(0,"]\n",sizeof("]"));
        }else if(i==14){
            write(0,"]\n",sizeof("]"));
        }else if(i==15){
            write(0,"]\n",sizeof("]"));
        }else if(i==16){
            write(0,"]\n",sizeof("]"));
        }else if(i==17){
            write(0,"]\n",sizeof("]"));
        }else if(i==18){
            write(0,"]\n",sizeof("]"));
        }else if(i==19){
            write(0,"]\n",sizeof("]"));
        }else{
            write(0,"]\n",sizeof("]\n"));
        }
    }
   sprintf(messaggio,YELLOW"Posizione [%d][%d] ID partita: [%d])\n"RESET,pos1,pos2,id_corrente);
   printf("w) Sopra  a) Sinistra  s) Sotto   d) Destra t) Tempo  i) Info c) Logout \n");
    stampa(messaggio);
} //fine funzione stampaMappa


//______________________________________________________________________________FUNZIONE

void sendSocket(int conn,char *mess,int nbytes)
{   //write su pipe
    if(write(conn,mess,nbytes)<0){
        error("Errore write\n");
    }
}



//______________________________________________________________________________FUNZIONE

void stampaVittoria(){
    system("clear");
    signal(SIGINT,sighandler);
    signal(SIGQUIT,sighandler);
    signal(SIGHUP,sighandler3);
    signal(SIGSTOP,sighandler3);
    signal(SIGTERM,sighandler3);
    signal(SIGPIPE,sighandler4);
    signal(SIGTSTP,sighandler3);
    stampa(GREEN"!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    stampa("* ASSURDO HAI VINTO !!! *\n");
    stampa("!!!!!!!!!!!!!!!!!!!!!!!!!!\n"RESET);
    sleep(1);
}

//______________________________________________________________________________FUNZIONE

void stampaSconfitta(){
    system("clear");
    signal(SIGINT,sighandler);
    signal(SIGQUIT,sighandler);
    signal(SIGHUP,sighandler3);
    signal(SIGSTOP,sighandler3);
    signal(SIGTERM,sighandler3);
    signal(SIGPIPE,sighandler4);
    signal(SIGTSTP,sighandler3);
    
    stampa(RED"*****************************\n");
    stampa("*  >>>> BOMBA ATOMICA <<<<  *\n");
    stampa("*****************************\n"RESET);
    sleep(1);
}

//______________________________________________________________________________FUNZIONE

void pauseT(){
    stampa("\nPremere Invio Per Continuare...\n");
    pulisciBuffer();
}

//______________________________________________________________________________FUNZIONE






























void sighandler(int segnale_ricevuto){
    char flag[3]; flag[0]='c';
    flag[1]=flagPosizione1;
    flag[2]=flagPosizione2;
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    write(connessioneFlag,flag,sizeof(flag));
    giocato=0; inGioco=0; sleep(1);
    write(connessioneFlag,"l",sizeof(char));
    exit(0);
}































void sighandler1(int segnale_ricevuto){
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    write(connessioneFlag,"l",1);
    giocato=0; inGioco=0; sleep(1);
    exit(0);
}
void sighandler2(int segnale_ricevuto){
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    write(connessioneFlag,"l",1);
    giocato=0; inGioco=0; sleep(1);
    exit(0);
}
void sighandler3(int segnale_ricevuto){
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    char flag[3]; flag[0]='c';
    flag[1]=flagPosizione1;
    flag[2]=flagPosizione2;
    write(connessioneFlag,flag,sizeof(flag));
    giocato=0; inGioco=0; sleep(1);
    write(connessioneFlag,"l",sizeof(char));
    sleep(1);
    exit(0);
}
void sighandler4(int segnale_ricevuto){
    stampa(RED"Il server è andato OFFLINE\n"RESET);
    close(connessioneFlag);
    exit(0);
}








