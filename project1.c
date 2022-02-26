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
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

//Defining macros

#define TRUE 1
#define FALSE 0

//"PRODUCE" message status indicates the signal for the producer to start producing based on the request from a consumer
#define PRODUCE 0

//"ACKNOWLEDGE" message indicates that the producer has produced data for a certain consumer
#define ACKNOWLEDGE 1

//"STOPPING_CRITERIA_STATUS" message status indicates that all the consumers have exited and so the producers should start exiting
#define STOPPING_CRITERIA_STATUS 2

//"SIZE" indicates the size of the buffer
#define SIZE 20

//The FIFO QUEUE
#define FIFO_QUEUE "./fifo_queue"
#define QUEUE_FULL "/Queue_full"
#define QUEUE_EMPTY "/Queue_empty"

//THE Binary semaphore lock
#define BINARY_SEMAPHORE_LOCK "/binary_semaphore_lock"

//Semaphore to keep count of the number of consumer processes which have exited
#define NUM_EXITED_CONSUMERS "/num_exited_consumers"

//Number of Producers
#define PRODUCERS_COUNT 6

//Number of Consumers
#define CONSUMERS_COUNT 8

//"STOPPING_CRITERIA" denotes the number of times a consumer will request and consume data
#define STOPPING_CRITERIA 5


sem_t *Queue_full;
sem_t *Queue_empty;
sem_t *binary_semaphore_lock;
sem_t *num_exited_consumers;

struct timeval starting_time, ending_time;

//Defining the message structure which we are going to use for message passing

struct message
{
    int data;
    int status;
    long time_stamp;
    int consumer_id;
    int producer_id;
    // int stop_c;
    
};

void producer(int pt_no)
{
    int PT_no = pt_no;

    // printing process number and producer id
    srand(time(NULL));
    printf("\nInside Producer Process --> Producer created with id: %d\n", PT_no);

    int Queue = open(FIFO_QUEUE, O_RDWR);
    struct message mesg;
    while (TRUE)
    {
        // int num_exited_cons;
        // printf("here");
        // sem_getvalue(num_exited_consumers, &num_exited_cons);
        // printf("\nNumber of exited consumers %d\n", num_exited_cons);
        // if (num_exited_cons==CONSUMERS_COUNT){
        //     break;
        // }
        // printf("producer waiting start %d\n", pt_no);
        sem_wait(Queue_full);
        // printf("producer waiting mutex start %d\n", pt_no);
        sem_wait(binary_semaphore_lock);
        ssize_t read_status = read(Queue, &mesg, sizeof(struct message));
        sem_post(Queue_empty);
        //sem_post(binary_semaphore_lock); printf("\nleft lock %d\n", pt_no);
        if (read_status > -1)
        {   
            // printf("\n%d message status\n", mesg.status);
            if (mesg.status == STOPPING_CRITERIA_STATUS){
                sem_wait(Queue_empty);
                write(Queue, &mesg, sizeof(struct message));
                sem_post(Queue_full);
                sem_post(binary_semaphore_lock);

                break;
            }
            if (mesg.status == PRODUCE)
            {
                // printf("producer producing %d\n", pt_no);
                sem_wait(Queue_empty);
                gettimeofday(&ending_time, NULL);
                // printf("producer producing mutex %d\n", pt_no);
                // sem_wait(binary_semaphore_lock);
                // printf("got lock  producing %d\n", pt_no);
                mesg.data = abs(rand()) + 1;
                mesg.status = ACKNOWLEDGE;
                long seconds = (ending_time.tv_sec - starting_time.tv_sec);
                long microseconds = ((seconds * 1000000) + ending_time.tv_usec) - (starting_time.tv_usec);
                mesg.time_stamp = microseconds;
                mesg.producer_id= PT_no;
                ssize_t done_write = write(Queue, &mesg, sizeof(struct message));
                if (done_write > -1)
                {
                    printf("\nInside Producer Process --> Producer with process number (assigned as producer id) %d produced data (Data= %d) for consumer process number (assigned as consumer id) %d as it received a PRODUCE request message (Message timestamp: [%ld microseconds])\n ", PT_no, mesg.data, mesg.consumer_id, mesg.time_stamp);
                    sem_post(Queue_full);
                }
                else
                {
                    printf("\nInside Producer Process --> Producer with process number %d failed to write to Queue\n", PT_no);
                }
                sem_post(binary_semaphore_lock); 
                //printf("\nleft lock %d\n", pt_no);
                //usleep(10);
            }
            else
            {
                // printf("producer  not producing %d\n", pt_no);
                sem_wait(Queue_empty);
                // printf("producer not producing lock %d\n", pt_no);
                // sem_wait(binary_semaphore_lock);
                // printf("got lock not producing %d\n", pt_no);
                 ssize_t done_write = write(Queue, &mesg, sizeof(struct message));
                if (done_write < 0){
                    printf("\nInside Producer Process --> Producer with process number %d failed to write to Queue\n", PT_no);
                }
                else
                {   
                    // printf ("\nInside Producer Process --> Producer with process number (assigned as producer id) %d received an ACKNOWLEDGEMT message from queue, hence pushed the message back to queue without doing anything\n", PT_no);
                    sem_post(Queue_full);
                }
                sem_post(binary_semaphore_lock);
                // printf("\nleft lock %d\n", pt_no);
                //usleep(10);
            }
        }
        else
        {
            printf("\nInside Producer Process --> Producer with process number %d failed to read from Queue\n", PT_no);
            sem_post(Queue_full);
            sem_post(binary_semaphore_lock); 
            //printf("\nleft lock %d\n", pt_no);
        }
    }
    close(Queue);
    printf("\nInside Producer Process --> Producer with process number %d exiting as all consumer work is done\n", PT_no);
    exit(EXIT_SUCCESS);
}

void consumer(int ct_no)
{
    int CT_no = ct_no;

    printf("\nInside Consumer Process --> Consumer created with id: %d \n", CT_no);
    int Queue = open(FIFO_QUEUE, O_RDWR);
    int request = 1; // request = 1 signifies that consumer will send a message requesting the producers to produce data for it by assigning the message status to PRODUCE
                    //request = 0 signifies that the consumer will go into message receiving anf consuming state
    struct message mesg;
    int stopping_criteria_request=0;
    int stopping_criteria_consume=0;
    while (TRUE)
    {
        if (request == 1)
        {   
            
            if (stopping_criteria_request==STOPPING_CRITERIA){
                //sem_post(binary_semaphore_lock);
                break;

            }
            // printf("request consumer waiting %d\n",ct_no);
            sem_wait(Queue_empty);
            // printf(" %d consumer requesting\n", ct_no);
            sem_wait(binary_semaphore_lock);
            // printf("got lock  consumer requesting %d\n", ct_no);
            gettimeofday(&ending_time, NULL); 
            long seconds = (ending_time.tv_sec - starting_time.tv_sec);
            long microseconds = ((seconds * 1000000) + ending_time.tv_usec) - (starting_time.tv_usec);
            mesg.time_stamp = microseconds;
            mesg.status = PRODUCE;
            mesg.data = 0;
            mesg.consumer_id = CT_no;
            mesg.producer_id=-999;
            // mesg.stop_c=0;
            ssize_t temp = write(Queue, &mesg, sizeof(struct message));
            if (temp < 0)
            {
                printf("\nInside Consumer Process --> Consumer with process number %d failed to send request to Queue\n", CT_no);
            }
            else
            {
                printf("\nInside Consumer Process --> Consumer with process number %d sent request to producers\n", CT_no);
                stopping_criteria_request+=1;
                sem_post(Queue_full);
            }
            sem_post(binary_semaphore_lock); //printf("\nleft lock %d\n", ct_no);
            request = 0; 
        }
        else
        {
            //printf("\nconsumed: %d\n",stopping_criteria_consume);
            if (stopping_criteria_consume==STOPPING_CRITERIA){
                //sem_post(binary_semaphore_lock);
                
                break;

            }
           
            sem_wait(Queue_full);
            //  printf("%d consumer reading and processing\n", ct_no);
            sem_wait(binary_semaphore_lock);
            // printf("got lock  consumer reading %d\n", ct_no);
            ssize_t read_msg = read(Queue, &mesg, sizeof(struct message));
            // printf("msg read %ld\n",read_msg);
            if (read_msg > -1)
            {
                // printf("properly read %d\n",ct_no);
                sem_post(Queue_empty);
                // printf("%d %d %d\n",mesg.id, CT_no, mesg.status);
                if (mesg.consumer_id == CT_no)
                {
                    if (mesg.status == ACKNOWLEDGE)
                    {
                        sem_post(binary_semaphore_lock); 
                        //printf("\nleft lock %d\n", ct_no);
                        printf("\nInside Consumer Process --> Consumer with process number (assigned as consumer id) %d received an ACKNOWLEDGEMENT MESSAGE with the same consumer id %d, hence consumed data from Queue, data= %d, producer id= %d (Message timestamp: [%ld microseconds])\n", CT_no,mesg.consumer_id, mesg.data,mesg.producer_id,mesg.time_stamp);
                        stopping_criteria_consume+=1;
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
                            // printf("\nInside Consumer Process --> Consumer with process number (assigned as consumer id) %d received a PRODUCE request message, hence did nothing to the message and pushed the message back to queue\n", CT_no);
                            sem_post(Queue_full);
                            sem_post(binary_semaphore_lock); 
                            //printf("\nleft lock %d\n", ct_no);
                        }
                        else
                        {
                            sem_post(binary_semaphore_lock);
                            // printf("\nleft lock %d\n", ct_no);
                            printf("\nInside Consumer Process --> Consumer with process number %d unable to access Queue\n", CT_no);
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
                        // printf("\nInside Consumer Process --> Consumer with process number (assigned as consumer id) %d received an ACKNOWLEDGEMENT MESSAGE with different consumer id %d, hence didn't consume the data and pushed the message back to queue\n", CT_no,mesg.consumer_id);
                        sem_post(Queue_full);
                        sem_post(binary_semaphore_lock);
                        // printf("\nleft lock %d\n", ct_no);
                    }
                    else
                    {
                        sem_post(binary_semaphore_lock); 
                        //printf("\nleft lock %d\n", ct_no);
                        printf("\nInside Consumer Process --> Consumer with process number %d unable to access Queue\n", CT_no);
                    }
                }
            }
            else
            {
                sem_post(binary_semaphore_lock);
                // printf("\nleft lock %d\n", ct_no);
                printf("\nInside Consumer Process --> Consumer with process number %d failed to read from Queue\n", CT_no);
            }
        }
        //usleep(50);
    }
    
    printf("\nInside Consumer Process --> Consumer with process number %d exiting\n", CT_no);
    sem_post(num_exited_consumers);
    int num_exited_cons;
    // printf("consumers here");
    sem_getvalue(num_exited_consumers, &num_exited_cons);
    printf("\nNumber of consumers exited%d\n", num_exited_cons);
    if (num_exited_cons==CONSUMERS_COUNT){
        // printf ("here");
        struct message mesg;
        mesg.status=STOPPING_CRITERIA_STATUS;
        sem_wait(binary_semaphore_lock);
	    sem_wait(Queue_empty);
	    write(Queue, &mesg, sizeof(struct message));
	    sem_post(Queue_full);
        sem_post(binary_semaphore_lock);
        // printf ("here 2");
        close(Queue);
        exit(EXIT_SUCCESS);
    }
    else{
        close(Queue);
    exit(EXIT_SUCCESS);
    }
}

int main()
{   
    //clearing up any data which might be stored previously
    unlink(FIFO_QUEUE);
    sem_unlink(QUEUE_EMPTY);
    sem_unlink(QUEUE_FULL);
    sem_unlink(BINARY_SEMAPHORE_LOCK);
    sem_unlink(NUM_EXITED_CONSUMERS);
    sem_destroy(Queue_empty);
    sem_destroy(Queue_full);
    sem_destroy(binary_semaphore_lock);
    sem_destroy(num_exited_consumers);
    //setting up the semaphores
    Queue_empty = sem_open(QUEUE_EMPTY, O_CREAT, 0777, SIZE);
    Queue_full = sem_open(QUEUE_FULL, O_CREAT, 0777, 0);
    binary_semaphore_lock = sem_open(BINARY_SEMAPHORE_LOCK, O_CREAT, 0777, 1);
    num_exited_consumers=sem_open(NUM_EXITED_CONSUMERS, O_CREAT, 0777, 0);
    int a,b,c,d;
    sem_getvalue(binary_semaphore_lock, &a);
    sem_getvalue(Queue_empty, &b);
    sem_getvalue(Queue_full, &c);
    sem_getvalue(num_exited_consumers, &d);
    printf("\nValues of binary_semaphore_lock = %d, Queue_empty = %d, Queue_full = %d\n", a,b,c );
    //getting time of the day
    gettimeofday(&starting_time, NULL);
    pid_t producer_id[PRODUCERS_COUNT], consumer_id[CONSUMERS_COUNT];

    const char *Queue = FIFO_QUEUE;
    //creating the Fifo queue
    mkfifo(Queue, 0666);

    int i, j, k;
    printf("\nSpawning child Processes for producers\n");
    for (i = 0; i < PRODUCERS_COUNT; i++)
    {
        producer_id[i] = fork(); //creating a child process
        if (producer_id[i] == 0) 
        {
            //if inside the child process then start the producer function and then break
            producer(i + 1);
            break;
        }
    }
    printf("\nSpawning child Processes for consumers\n");
     for (j = 0; j < CONSUMERS_COUNT; j++)
    {
        consumer_id[j] = fork(); //creating a child process
        if (consumer_id[j] == 0)
        {
            //if inside the child process then start the consumer function and then break
            consumer(j + 1);
            break;
        }
    }

    // This for loop with the wait(NULL) statement will move on only when a process ends, 
    // so it waits until PRODUCERS_COUNT + CONSUMERS_COUNT number of processes ends. 

    for (k = 0; k < PRODUCERS_COUNT + CONSUMERS_COUNT; k++)
        wait(NULL);

    printf("\nAll producer and consumer processes terminated\n");
    //clearing up all the stored data 
    unlink(FIFO_QUEUE);
    sem_unlink(QUEUE_EMPTY);
    sem_unlink(QUEUE_FULL);
    sem_unlink(BINARY_SEMAPHORE_LOCK);
    sem_unlink(NUM_EXITED_CONSUMERS);
    sem_destroy(Queue_empty);
    sem_destroy(Queue_full);
    sem_destroy(binary_semaphore_lock);
    sem_destroy(num_exited_consumers);
    return 0;
}