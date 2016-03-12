#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define USER 2            //生产者数量，消费者数量
#define NUMS 10

typedef struct{
	int *buf;         //缓冲区
	int n;              //缓冲区最大长度
	int front;           //队头 (front+1)%n 是第一个元素 
	int rear;            //队尾  (rear % n)是最后一个元素
	sem_t mutex;          //保护对缓冲区的访问
	sem_t slots;          //Counts the slot
	sem_t items;          //Counts the item    
}sbuf_t;

void sbuf_init(sbuf_t *sp, int n){
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

void sbuf_insert(sbuf_t *sp, int item){
	int ret;

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
	
	pthread_t tid;
	tid = pthread_self();
	sp->buf[sp->rear] = item;
	sp->rear = ++(sp->rear) % sp->n;
	
	printf("thread[%u] insert an item :%d\n", (unsigned int)tid, item);
	
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

int sbuf_remove(sbuf_t * sp){
	int item = -1;
	int ret;

	ret = sem_wait(&sp->items);
	if (ret != 0){
		fprintf(stderr, "sem_wait items error\n");
		exit(EXIT_FAILURE);
	}

	ret = sem_wait(&sp->mutex);
	if (ret != 0){
		fprintf(stderr, "sem_wait mutex error\n");
		exit(EXIT_FAILURE);					
	}
	
	pthread_t tid;
	tid = pthread_self();
	item = sp->buf[sp->front];
	sp->front = (++sp->front) % sp->n;
	printf("thread[%u] delete an item :%d\n", (unsigned int)tid, item);

	ret = sem_post(&sp->mutex);
	if(ret != 0){
		fprintf(stderr, "sem_post mutex error\n");
		exit(EXIT_FAILURE);
	}
	ret = sem_post(&sp->slots);
	if(ret != 0){
		fprintf(stderr, "sem_post items error\n");
		exit(EXIT_FAILURE);
	}
	return item;
}

void *producer(void *arg){
	sbuf_t *sbp = (sbuf_t *) arg;
	int randb;
	srand((unsigned)time(NULL));
	while(1){
		randb = rand();
		sleep(rand()%2);
		//printf("now produce want to produce %d \n",randb);
		//if(sbuf_insert(sbp, randb)){
			//printf("report error condition of producer\n\n");
		//}else{
			sbuf_insert(sbp, randb);
			//printf("producer produced %d\n\n",randb);
		//}
	}
}

void *consumer(void *arg){
	sbuf_t *sbp = (sbuf_t *) arg;
	int item;
	srand((unsigned)time(NULL));
	while(1){	
		sleep(rand()%2);
		//printf("now consume want to consume an item\n");
		if((item = sbuf_remove(sbp)) < 0)
			printf("report error condition of consume\n\n");
		//else
			//printf("consume consumed %d\n\n",item);
	}
}

int main(int argc, char *argv[]){
	pthread_t tidProducer[USER], tidConsumer[USER];
	int i;
	int err;

	sbuf_t sb;

	sbuf_init(&sb, NUMS);

	for( i = 0 ; i < USER; ++i){
		err = pthread_create(&tidProducer[i], NULL, producer, (void*)(&sb));
		if (err != 0){
			fprintf(stderr, "pthread_create error\n");
			exit(EXIT_FAILURE);
		}
	}

	for(i = 0; i < USER; ++i)
	err = pthread_create(&tidConsumer[i], NULL, consumer, (void*)(&sb));
	if(err != 0){
		fprintf(stderr, "pthread_create error\n");
		exit(EXIT_FAILURE);
	}


	sleep(10);
	for (i = 0; i < USER; i++){
		err = pthread_kill(tidProducer[i], 0);
		if(err != 0){
			fprintf(stderr, "pthread_kill error\n");\
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < USER; i++){
		err = pthread_kill(tidConsumer[i], 0);
		if(err != 0){
			fprintf(stderr, "pthread_kill error\n");\
			exit(EXIT_FAILURE);
		}
	}
	
	sbuf_deinit(&sb);
	return 0;
}
