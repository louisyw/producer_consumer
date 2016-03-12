/*
 *使用信号量来解决生产者消费者问题,缓冲区为全局变量
 *Author: louisyw
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NUMS 100

typedef struct{
	int *buf;         //缓冲区
	int n;              //缓冲区最大长度
	int front;           //队头 (front+1)%n 是第一个元素 
	int rear;            //队尾  (rear % n)是最后一个元素
	sem_t mutex;          //保护对缓冲区的访问
	sem_t slots;          //Counts the slot
	sem_t items;          //Counts the item    
}sbuf_t;

sbuf_t sb;
sbuf_t *sp = &sb;

void sbuf_init(int n){
	int result;
	sp->buf = (int*)calloc(n, sizeof(int));
	if(sp->buf == NULL){
		fprintf(stderr, "calloc error\n");
		exit(EXIT_FAILURE);
	}
	sp->n = n;
	sp->front = sp->rear = 0;
	
	result = sem_init(&sp->mutex, 0, 1);   
	if (result != 0){
		fprintf(stderr, "sem_init mutex error\n");
		exit(EXIT_FAILURE);
	}
	
	result = sem_init(&sp->slots, 0, n);
	if (result != 0){
		fprintf(stderr, "sem_init slots error\n");
		exit(EXIT_FAILURE);
	}

	result = sem_init(&sp->items, 0, 0);
	if ( result != 0){
		fprintf(stderr, "sem_init items error\n");
		exit(EXIT_FAILURE);
	}
}

void sbuf_deinit(sbuf_t *sp){
	free(sp->buf);
	sp->buf = NULL;
}

void sbuf_insert(int item){
	int ret;
	int i;
	ret = sem_wait(&sp->slots);
	if (ret != 0){
		fprintf(stderr, "sem_wait slots error\n");
		exit(EXIT_FAILURE);
	}
	ret = sem_wait(&sp->mutex);
	if (ret != 0){
		fprintf(stderr, "sem_wait mutex error\n");         
		exit(EXIT_FAILURE);					
	}
	
	sp->buf[sp->rear] = item;
	sp->rear = (++sp->rear) % sp->n;
	printf("insert an item :%d\n", item);
	
	ret = sem_post(&sp->mutex);
	if(ret != 0){	            
		fprintf(stderr, "sem_post mutex error\n");										
		exit(EXIT_FAILURE);
	}
	ret = sem_post(&sp->items);
	if(ret != 0){          
		fprintf(stderr, "sem_post items error\n"); 
		exit(EXIT_FAILURE);									
	}
}

int sbuf_remove(){
	int ret;
	int item = -1;
	
	ret = sem_wait(&sp->items);
	if ( ret != 0){
		fprintf(stderr, "sem_wait items error\n");
		exit(EXIT_FAILURE);
	}

	ret = sem_wait(&sp->mutex);
	if ( ret != 0){
		fprintf(stderr, "sem_wait mutex error\n");
		exit(EXIT_FAILURE);					
	}

	item = sp->buf[sp->front];
	sp->front = (++sp->front) % sp->n;

	printf("delete an item :%d\n", item);
	
	ret = sem_post(&sp->mutex);
	if( ret != 0){
		fprintf(stderr, "sem_post mutex error\n");
		exit(EXIT_FAILURE);
	}
	ret = sem_post(&sp->slots);
	if( ret != 0){
		fprintf(stderr, "sem_post items error\n");
		exit(EXIT_FAILURE);
	}

	return item;
}

void *producer(void *arg){
	int randb;
	srand((unsigned)time(NULL));
	while(1){
		randb = rand();
		sleep(rand()%2);
		printf("now produce want to produce %d \n",randb);
		//if(sbuf_insert(sbp, randb)){
		//	printf("report error condition of producer\n\n");
		//}else{
			sbuf_insert(randb);
			printf("producer produced %d\n\n",randb);
		//}
	}
}

void *consumer(void *arg){
	int item;
	sleep(2);
	srand((unsigned)time(NULL));
	while(1){	
		sleep(rand()%2);
		printf("now consumer want to consume an item\n");
		if((item = sbuf_remove()) < 0)
			printf("report error condition of consume\n\n");
		else
			printf("consume consumed %d\n\n",item);
	}
}

int main(int argc, char *argv[]){
	pthread_t tidProducer, tidConsumer;
	int i;
	int err;

	sbuf_init(NUMS);

	err = pthread_create(&tidProducer, NULL, producer, (void*)(&sb));
	if (err != 0){
		fprintf(stderr, "pthread_create error\n");
		exit(EXIT_FAILURE);
	}
	
	err = pthread_create(&tidConsumer, NULL, consumer, (void*)(&sb));
	if(err != 0){
		fprintf(stderr, "pthread_create error\n");
		exit(EXIT_FAILURE);
	}

	sleep(10);
	pthread_kill(tidProducer, 0);
	pthread_kill(tidConsumer, 0);
	
	sbuf_deinit(sp);
	return 0;
}
