// Name: Rohan Chaudhury
// CSCE 611 600: OPERATING SYSTEMS Project #1
// Email Address: rohan.chaudhury@tamu.edu
// UIN: 432001358
// Collaborated with: Abhishek Sinha, Rohit Sah, Shubham Gupta, Sherine Davis Kozhikadan

// Importing the necessary libraries

#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>

#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0
#define PRODUCE 0
#define ACKNOWLEDGE 1
#define SIZE 20
#define FIFO_QUEUE "./fifo_queue"
#define QUEUE_FULL "/Queue_full"
#define QUEUE_EMPTY "/Queue_empty"
#define MUTEX_LOCK "/mutex_lock"
#define PRODUCERS_COUNT 10
#define CONSUMERS_COUNT 15

sem_t *Queue_full;
sem_t *Queue_empty;
sem_t *mutex_lock;

struct message
{
    int data;
    int status;
    time_t time_stamp;
    int id;
};

void producer(int pt_no)
{
    int PT_no = pt_no;

    // printing process number and producer id
    srand(time(NULL));
    printf("\nProducer created with id: %d\n", PT_no);

    int Queue = open(FIFO_QUEUE, O_RDWR);
    struct message mesg;
    while (TRUE)
    {
        // printf("producer waiting start %d\n", pt_no);
        sem_wait(Queue_full);
        // printf("producer waiting mutex start %d\n", pt_no);
        sem_wait(mutex_lock);
        ssize_t read_status = read(Queue, &mesg, sizeof(struct message));
        sem_post(Queue_empty);
        //sem_post(mutex_lock); printf("\nleft lock %d\n", pt_no);
        if (read_status > -1)
        {
            if (mesg.status == PRODUCE)
            {
                // printf("producer producing %d\n", pt_no);
                sem_wait(Queue_empty);
                // printf("producer producing mutex %d\n", pt_no);
                // sem_wait(mutex_lock);
                // printf("got lock  producing %d\n", pt_no);
                mesg.data = abs(rand()) + 1;
                mesg.status = ACKNOWLEDGE;
                mesg.time_stamp = time(NULL);
                ssize_t done_write = write(Queue, &mesg, sizeof(struct message));
                if (done_write > -1)
                {
                    printf("\nProducer with process number %d produced data for request id %d. Data= %d\n", PT_no, mesg.id, mesg.data);
                    sem_post(Queue_full);
                }
                else
                {
                    printf("\nProducer with process number %d failed to write to Queue\n", PT_no);
                }
                sem_post(mutex_lock); //printf("\nleft lock %d\n", pt_no);
                //usleep(10);
            }
            else
            {
                // printf("producer  not producing %d\n", pt_no);
                sem_wait(Queue_empty);
                // printf("producer not producing lock %d\n", pt_no);
                // sem_wait(mutex_lock);
                // printf("got lock not producing %d\n", pt_no);
                 ssize_t done_write = write(Queue, &mesg, sizeof(struct message));
                if (done_write < 0){
                    printf("\nProducer with process number %d failed to write to Queue\n", PT_no);
                }
                else
                {
                    sem_post(Queue_full);
                }
                sem_post(mutex_lock);// printf("\nleft lock %d\n", pt_no);
                //usleep(10);
            }
        }
        else
        {
            printf("\nProducer with process number %d failed to read from Queue\n", PT_no);
            sem_post(Queue_full);
            sem_post(mutex_lock); //printf("\nleft lock %d\n", pt_no);
        }
    }
    exit(0);
}

void consumer(int ct_no)
{
    int CT_no = ct_no;

    printf("\nConsumer created with id: %d \n", CT_no);
    int Queue = open(FIFO_QUEUE, O_RDWR);
    int request = 1;
    struct message mesg;
    while (TRUE)
    {
        if (request == 1)
        {
            // printf("request consumer waiting %d\n",ct_no);
            sem_wait(Queue_empty);
            // printf(" %d consumer requesting\n", ct_no);
            sem_wait(mutex_lock);
            // printf("got lock  consumer requesting %d\n", ct_no);
            
            mesg.time_stamp = time(NULL);
            mesg.status = PRODUCE;
            mesg.data = 0;
            mesg.id = CT_no;
            ssize_t temp = write(Queue, &mesg, sizeof(struct message));
            if (temp < 0)
            {
                printf("\nConsumer with process number %d failed to send request to Queue\n", CT_no);
            }
            else
            {
                printf("\nConsumer with process number %d sent request to producers\n", CT_no);
                sem_post(Queue_full);
            }
            sem_post(mutex_lock); //printf("\nleft lock %d\n", ct_no);
            request = 0;
        }
        else
        {
           
            sem_wait(Queue_full);
            //  printf("%d consumer reading and processing\n", ct_no);
            sem_wait(mutex_lock);
            // printf("got lock  consumer reading %d\n", ct_no);
            ssize_t read_msg = read(Queue, &mesg, sizeof(struct message));
            // printf("msg read %ld\n",read_msg);
            if (read_msg > -1)
            {
                // printf("properly read %d\n",ct_no);
                sem_post(Queue_empty);
                // printf("%d %d %d\n",mesg.id, CT_no, mesg.status);
                if (mesg.id == CT_no)
                {
                    if (mesg.status == ACKNOWLEDGE)
                    {
                        sem_post(mutex_lock); //printf("\nleft lock %d\n", ct_no);
                        printf("\nConsumer with process number %d consumed data from Queue with request id %d, data= %d\n", CT_no,mesg.id, mesg.data);
                        request = 1;
                        // printf("\nConsumer with process number %d exiting\n", CT_no);
                        
                    }
                    else
                    {
                        // printf("not acknowledged waiting %d\n",ct_no);
                        sem_wait(Queue_empty);
                        ssize_t conWrite = write(Queue, &mesg, sizeof(struct message));
                        if ( conWrite > -1)
                        {
                            sem_post(Queue_full);
                            sem_post(mutex_lock); //printf("\nleft lock %d\n", ct_no);
                        }
                        else
                        {
                            sem_post(mutex_lock);// printf("\nleft lock %d\n", ct_no);
                            printf("\nConsumer with process number %d unable to access Queue\n", CT_no);
                        }
                    }
                }
                else
                {
                    // printf("not ours waiting %d\n",ct_no);
                    sem_wait(Queue_empty);
                    ssize_t conWrite = write(Queue, &mesg, sizeof(struct message));
                    if ( conWrite > -1)
                    {
                        sem_post(Queue_full);
                        sem_post(mutex_lock);// printf("\nleft lock %d\n", ct_no);
                    }
                    else
                    {
                        sem_post(mutex_lock); //printf("\nleft lock %d\n", ct_no);
                        printf("\nConsumer with process number %d unable to access Queue\n", CT_no);
                    }
                }
            }
            else
            {
                sem_post(mutex_lock);// printf("\nleft lock %d\n", ct_no);
                printf("\nConsumer with process number %d failed to read from Queue\n", CT_no);
            }
        }
        //usleep(50);
    }
    close(Queue);
    exit(0);
}

int main()
{
    unlink(FIFO_QUEUE);
    sem_unlink(QUEUE_EMPTY);
    sem_unlink(QUEUE_FULL);
    sem_unlink(MUTEX_LOCK);
    sem_destroy(Queue_empty);
    sem_destroy(Queue_full);
    sem_destroy(mutex_lock);
    Queue_empty = sem_open(QUEUE_EMPTY, O_CREAT, 0777, SIZE);
    Queue_full = sem_open(QUEUE_FULL, O_CREAT, 0777, 0);
    mutex_lock = sem_open(MUTEX_LOCK, O_CREAT, 0777, ACKNOWLEDGE);
    pid_t producer_id[PRODUCERS_COUNT], consumer_id[CONSUMERS_COUNT];
    // int val;
    // sem_getvalue(mutex_lock, &val);
    // printf("val %d\n", val);
    const char *Queue = FIFO_QUEUE;
    mkfifo(Queue, 0666);

    int i, j, k;
    
    for (i = 0; i < PRODUCERS_COUNT; i++)
    {
        producer_id[i] = fork();
        if (producer_id[i] == 0)
        {
            producer(i + 1);
            break;
        }
    }

     for (j = 0; j < CONSUMERS_COUNT; j++)
    {
        consumer_id[j] = fork();
        if (consumer_id[j] == 0)
        {
            consumer(j + 1);
            break;
        }
    }

    for (k = 0; k < PRODUCERS_COUNT + CONSUMERS_COUNT; k++)
        wait(NULL);

    printf("\nAll producer and consumer processes terminated\n");
    unlink(FIFO_QUEUE);
    sem_unlink(QUEUE_EMPTY);
    sem_unlink(QUEUE_FULL);
    sem_destroy(Queue_empty);
    sem_destroy(Queue_full);
    sem_destroy(mutex_lock);
    return 0;
}