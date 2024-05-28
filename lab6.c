#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <math.h>

int primos = 0, tamBuf, *buffer, valBufP = 0, *nPrimos; //Variaveis globais
sem_t sem; //semaforo para incrementar n de primos
sem_t cheio, vazio; //sinaliza buffer cheio ou vazio
sem_t mcons; //exclusao mutua entre consumidores
size_t tamArq = 1000;

typedef struct{
    char *arquivo;
} tProd;

void insere(){

}

void * carregaProd(void * arg){
    size_t lei;
    int in = 0;
    tProd * args = (tProd *) arg;

    //Abre o arquivo binario
    FILE * dArq = fopen(args->arquivo, "rb"); //Descritor do arquivo de entrada
    if(!dArq){
        printf("Erro ao abrir arquivo\n");
        pthread_exit(NULL);
    }

    //Verifica a quantidade de inteiros existem no arquivo
    fseek(dArq, 0, SEEK_END);
    tamArq = ftell(dArq);
    fseek(dArq, 0, SEEK_SET);
    tamArq = tamArq / sizeof(int);

    //Aloca memoria para o buffer
    buffer = (int*) malloc(sizeof(int) * tamArq);
    if(!buffer){
        printf("Erro ao alocar memoria\n");
        pthread_exit(NULL);
    }

    //Escreve no buffer
    for(int i = 0; i < tamArq; i++){
        sem_wait(&vazio);
        lei = fread(&buffer[in], sizeof(int), 1, dArq);
        in = (in + 1) % tamArq;
        sem_post(&cheio);
    }
}

/*Funcao das threads consumidoras, se o numero for primo
  a variavel global primo e nPrimos[idThread] serao incrementados*/
void * ehPrimoCon(void * arg){
    static int valBufC = 0, total = 0;
    int idThread = (int) arg;
    int temp;
    int i, j;
    nPrimos[idThread] = 0;
    for(;total < tamArq;){
        j = 0;
        sem_wait(&cheio);
        sem_wait(&mcons);
        if (buffer[valBufC] == 2){
            valBufC = (valBufC + 1) % tamArq;
            sem_post(&mcons);
            nPrimos[idThread]++;
            sem_wait(&sem);     //Bloqueia outras threads que chegarem na secao critica
            primos++;
            sem_post(&sem);     //Pode desbloquear uma thread que chegou na secao critica
        }
        else if(buffer[valBufC] != 1){
            temp = buffer[valBufC];
            valBufC = (valBufC + 1) % tamArq;
            sem_post(&mcons);
            for(i=3; i < sqrt(temp)+1; i+=2){ //Verifica se eh um numero primo != de 2
                if(temp % i == 0){
                    i = sqrt(temp);
                    j = 1;
                }
            }
            if(j == 0){
                nPrimos[idThread]++;
                sem_wait(&sem); //Bloqueia outras threads que chegarem na secao critica
                primos++;
                sem_post(&sem); //Pode desbloquear uma thread que chegou na secao critica
            }
        }
        total++;
        sem_post(&vazio);
    }
}



int main (int argc, char*argv[]){
    int tCon; //Num de threads
    int idP = 0;

    tProd *args;
    args = malloc(sizeof(tProd));
    if(!args){
        printf("Erro ao alocar memoria\n");
        return -1;
    }


    //Verifica se todas as entradas estao presentes
    if(argc < 4){
        printf("Digite: %s <NumDeThreads> <TamDoBuffer> <ArquivoDeEntrada>\n", argv[0]);
        return 1;
    }

    //Atribui o numero de threads a variavel tCon e verifica se eh um valor valido
    tCon = atol(argv[1]);
    if(tCon < 1){
        printf("Numero de threads consumidoras deve ser no minimo 1\n");
        return 2;
    }
    int idC[tCon];

        nPrimos = malloc(sizeof(int) * tCon);
    if(!nPrimos){
        printf("Erro ao alocar memoria\n");
        return -1;
    }

    //Atribui o tamanho do buffer a variavel tamBuf e verifica se eh um valor valido
    tamBuf = atol(argv[2]);
    if(tamBuf < 1){
        printf("O tamanho do buffer deve ser no minimo 1\n");
        return 2;
    }

    args->arquivo = argv[3];

    sem_init(&sem, 0, 1);
    sem_init(&cheio, 0, 0);
    sem_init(&vazio, 0, tamBuf);
    sem_init(&mcons, 0, 1);

    if(pthread_create(&idP, NULL, carregaProd, (void*) args)){
        printf("Erro ao criar thread\n");
        return 3;
    }

    for(int i = 0; i<tCon; i++){
        if(pthread_create(&idC[i], NULL, ehPrimoCon, (void*)i)){
            printf("Erro ao criar thread\n");
            return 3;
        }
    }
    pthread_join(idP, NULL);
    for (int a=0; a<tCon; a++) {
        if (pthread_join(idC[a], NULL)) {
            printf("Erro pthread_join()\n");
            return -2;
        }
    }
}
