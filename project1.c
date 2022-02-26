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

//A message can have the following 3 status: PRODUCE, ACKNOWLEDGE, STOPPING_CRITERIA_STATUS 

//"PRODUCE" message status indicates the signal for the producer to start producing based on the request from a consumer
#define PRODUCE 0

//"ACKNOWLEDGE" message indicates that the producer has produced data for a certain consumer
#define ACKNOWLEDGE 1

//"STOPPING_CRITERIA_STATUS" message status indicates that all the consumers have exited and so the producers should start exiting
#define STOPPING_CRITERIA_STATUS 2

//"SIZE" indicates the size of the fifo buffer/queue used for message passing
#define SIZE 5

//The FIFO QUEUE/BUFFER
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

//Semphores for managing the FIFO Queue
//Queue_full semaphore gives the number of full slots in the Queue/Buffer
sem_t *Queue_full;
//Queue_empty semaphore gives the number of empty slots in the Queue/Buffer
sem_t *Queue_empty;

//THE Binary semaphore lock for mutual exclusion
sem_t *binary_semaphore_lock;

//Semaphore to keep count of the number of consumer processes which have exited
sem_t *num_exited_consumers;

struct timeval starting_time, ending_time;

//Defining the message structure which we are going to use for message passing

struct message
{
    //the data produced by producer
    int data;
    //status of the message
    int status;
    //timestamp of the message in microseconds calculated from the beginning of execution of the program
    long time_stamp;

    //consumer_id denotes the consumer process number which originated the message request
    int consumer_id;

    //producer_id denotes the producer process number which produced the data
    int producer_id;

    // int stop_c;
    
};

//Defining the producer function

void producer(int pt_no)
{   
    //pt_no gives the process number of the producer process which we are also using as producer id
    int PT_no = pt_no;

    
    srand(time(NULL));

    // printing process number/producer id
    printf("\nInside Producer Process --> Producer created with id: %d\n", PT_no);

    //Opening the FIFO QUEUE
    int Queue = open(FIFO_QUEUE, O_RDWR);

    struct message mesg;
    
    while (TRUE)
    {
        //The producer processes will stay in this loop until they encounter a message with status as "STOPPING_CRITERIA_STATUS"

        // printf("here");
        // printf("producer waiting start %d\n", pt_no);
        sem_wait(Queue_full);
        // printf("producer waiting mutex start %d\n", pt_no);
        sem_wait(binary_semaphore_lock);

        //Reading the front value of the Queue/BUFFER

        ssize_t read_status = read(Queue, &mesg, sizeof(struct message));


        if (read_status > -1) //If successfully read the data
        {   

            // printf("\n%d message status\n", mesg.status);

            if (mesg.status == STOPPING_CRITERIA_STATUS){ 
                
                //If message status = STOPPING_CRITERIA_STATUS then the producer process should exit after pushing
                //the same message back to the queue so that the other producer processes can exit
                
                write(Queue, &mesg, sizeof(struct message));
                sem_post(Queue_full);
                sem_post(binary_semaphore_lock);

                break;
            }
            if (mesg.status == PRODUCE)
            {
                
                //If message status= PRODUCE then the producer should produce data and write it to Queue/BUFFER

                // printf("producer producing %d\n", pt_no);


                //Getting the time to calculate the time stamp
                gettimeofday(&ending_time, NULL);

                //Producing random data and assigning it to the data variable of the message
                mesg.data = abs(rand()) + 1;

                //Changing the status of the message to ACKNOWLEDGE so that the consumers can consume this message
                mesg.status = ACKNOWLEDGE;

                //calculating the microseconds since the start of the program execution
                long seconds = (ending_time.tv_sec - starting_time.tv_sec);
                long microseconds = ((seconds * 1000000) + ending_time.tv_usec) - (starting_time.tv_usec);

                //assigning the microseconds value to time_stamp variable of the message
                mesg.time_stamp = microseconds;

                //Assigning the producer id/process number to the producer_id variable of the message
                mesg.producer_id= PT_no;

                //Writing the message onto the Queue/Buffer
                ssize_t done_write = write(Queue, &mesg, sizeof(struct message));
                if (done_write > -1)
                {
                    //If write is successfully done then we print the necessary log statements and 
                    //increment the Queue_full semaphore by 1.
                    printf("\nInside Producer Process --> Producer with process number (assigned as producer id) %d produced data (Data= %d) for consumer process number (assigned as consumer id) %d as it received a PRODUCE request message (Message timestamp: [%ld microseconds])\n ", PT_no, mesg.data, mesg.consumer_id, mesg.time_stamp);
                    sem_post(Queue_full);
                }
                else
                {
                    //If write fails then indicating that with a print statement
                    printf("\nInside Producer Process --> Producer with process number %d failed to write to Queue\n", PT_no);
                }

                //Releasing the binary_semaphore_lock after the required operations are complete
                sem_post(binary_semaphore_lock); 
                //printf("\nleft lock %d\n", pt_no);
                //usleep(10);
            }
            else
            {
                //The process enters here when the read messsage has the status ACKNOWLEDGE which is meant for a consumer
                //Then we simply push back this message onto the queue and release the binary_semaphore_lock
                



                ssize_t done_write = write(Queue, &mesg, sizeof(struct message));
                if (done_write < 0){

                    //Write operation failed
                    printf("\nInside Producer Process --> Producer with process number %d failed to write to Queue\n", PT_no);
                }
                else
                {   
                    //Write successfully done

                    // printf ("\nInside Producer Process --> Producer with process number (assigned as producer id) %d received an ACKNOWLEDGEMT message from queue, hence pushed the message back to queue without doing anything\n", PT_no);
                    sem_post(Queue_full);
                }
                sem_post(binary_semaphore_lock);
                // printf("\nleft lock %d\n", pt_no);

            }
        }
        else
        {
            //Queue read operation failed for the process

            printf("\nInside Producer Process --> Producer with process number %d failed to read from Queue\n", PT_no);
            sem_post(Queue_full);
            sem_post(binary_semaphore_lock); 
            //printf("\nleft lock %d\n", pt_no);
        }
    }

    //We are here, outside the while(True) loop when the process is ready to exit after
    //all the consumer processes have exited

    //Closing the Queue before exiting
    close(Queue);
    printf("\nInside Producer Process --> Producer with process number %d exiting as all consumer work is done and all the consumer processes have exited\n", PT_no);
    
    //Producer process exiting
    exit(EXIT_SUCCESS);
}


//Defining the consumer function

void consumer(int ct_no)
{
    //CT_no is the consumer process number which we are also using as consumer id
    int CT_no = ct_no;

    printf("\nInside Consumer Process --> Consumer created with id: %d \n", CT_no);

    //Opening the Queue/Buffer
    int Queue = open(FIFO_QUEUE, O_RDWR);
    int request = 1; // request = 1 signifies that consumer will send a message requesting the producers to produce data for it by assigning the message status to PRODUCE
                    //request = 0 signifies that the consumer will go into message receiving anf consuming state
    struct message mesg;

    //Defining 2 int variables which will determine when the consumer should stop requesting and consuming
    int stopping_criteria_request=0;
    int stopping_criteria_consume=0;
    while (TRUE)
    {
        if (request == 1) //Here the consumer is in the requesting state
        {   
            
            if (stopping_criteria_request==STOPPING_CRITERIA){
                // If stopping_criteria_request==STOPPING_CRITERIA then the consumer should stop requesting 
                // and the consumer process should exit

                break;

            }


            //Checking if the Queue has atleast 1 empty slot before writing the
            //request message onto the Queue
            sem_wait(Queue_empty);

            //waiting to acquire the binary_semaphore_lock
            sem_wait(binary_semaphore_lock);

            // printf("got lock consumer requesting %d\n", ct_no);
            gettimeofday(&ending_time, NULL); 
            long seconds = (ending_time.tv_sec - starting_time.tv_sec);
            long microseconds = ((seconds * 1000000) + ending_time.tv_usec) - (starting_time.tv_usec);
            mesg.time_stamp = microseconds;

            //Assigning the status variable of the message to PRODUCE so that when the producer 
            //receives this message, it starts producing

            mesg.status = PRODUCE;
            mesg.data = 0;

            //Assigning the consumer id to the consumer_id variable of the message
            mesg.consumer_id = CT_no;

            //assigning a garbage value to producer_id variable since this message has not yet reached any producer
            mesg.producer_id=-999;
            // mesg.stop_c=0;

            //Attempting to write the request on Queue
            ssize_t temp = write(Queue, &mesg, sizeof(struct message));
            if (temp < 0)
            {
                //Write operation failed if temp<0
                printf("\nInside Consumer Process --> Consumer with process number %d failed to send request to Queue\n", CT_no);
            }
            else
            {
                //Write operation was successfull
                //Logging the necessary data
                printf("\nInside Consumer Process --> Consumer with process number %d sent request to producers\n", CT_no);
                
                //Incrementing stopping_criteria_request by 1 since the consumer has requested once now
                stopping_criteria_request+=1;
                //Incrementing Queue_full semaphore by 1 since 1 message was written to Queue
                sem_post(Queue_full);
            }

            //Releasing the binary_semaphore_lock
            sem_post(binary_semaphore_lock); //printf("\nleft lock %d\n", ct_no);

            //Changing the state of the consumer process from requesting to consuming
            //request=0 indicates that the consumer process should start consuming
            request = 0; 
        }
        else
        {
            //printf("\nconsumed: %d\n",stopping_criteria_consume);
            if (stopping_criteria_consume==STOPPING_CRITERIA){

                // If stopping_criteria_consume==STOPPING_CRITERIA then the consumer should stop consuming
                // and the consumer process should exit               
                break;

            }
           

            //Waiting for the Queue to have atleast 1 message to be read
            sem_wait(Queue_full);
            //  printf("%d consumer reading and processing\n", ct_no);

            //Acquiring the binary_semaphore_lock
            sem_wait(binary_semaphore_lock);
            // printf("got lock consumer reading %d\n", ct_no);

            //Attempting to read data from the front of thq Queue
            ssize_t read_msg = read(Queue, &mesg, sizeof(struct message));

            if (read_msg > -1)
            {
                //If read_msg > -1 then message was properly read

                // printf("%d %d %d\n",mesg.id, CT_no, mesg.status);
                sem_post(Queue_empty);
                sem_post(binary_semaphore_lock);
                //A consumer process will only consume the message which has the same consumer_id as the consumer process
                //The consumer_id was assigned to the message by the consumer process when it had written the request meant for the
                //producers onto the Queue
                if (mesg.consumer_id == CT_no) //Checking if the consumer_id variable of the message matches with the consumer id of the process
                {
                    //When it matches we execute this part of the code
                    if (mesg.status == ACKNOWLEDGE) 
                    {
                        //mesg.status == ACKNOWLEDGE indicates that the request message was acknowledged by the 
                        //producer and it had written the data onto the message

                        //since message is read by the consumer so the consumer process can relinquish the binary_semaphore_lock

                        
                        //Since 1 message is read we are incrementing Queue_empty by 1
                        
                         
                        

                        //Consumer processing the acquired data and logging the necessary values
                        printf("\nInside Consumer Process --> Consumer with process number (assigned as consumer id) %d received an ACKNOWLEDGEMENT MESSAGE with the same consumer id %d, hence consumed data from Queue, data= %d, producer id= %d (Message timestamp: [%ld microseconds])\n", CT_no,mesg.consumer_id, mesg.data,mesg.producer_id,mesg.time_stamp);
                        
                        //Incrementing stopping_criteria_consume by 1 since the consumer has consumed once now
                        stopping_criteria_consume+=1;

                        //Changing the state of the consumer process from consuming to requesting 
                        //request=1 indicates that the consumer process should request for new data now
                        request = 1;
                        
                        
                    }
                    else
                    {
                        //The process enters here when mesg.status == PRODUCE
                        //that means that this read message has not yet been acknowledged by the producer
                        //hence the consumer process will write the message back to the Queue for the producer
                        //to read without making any changes
                        
                        //Checking if the the Queue has atleast 1 empty slot before writing
                        // sem_wait(Queue_empty);

                        //Attempting to write to Queue
                        sem_wait(Queue_empty);
                        sem_wait(binary_semaphore_lock);
                        ssize_t conWrite = write(Queue, &mesg, sizeof(struct message));
                        if ( conWrite > -1)
                        {
                            //Write operation successfull
                            //then the consumer process increments Queue_full by 1 and releases the binary_semaphore_lock

                            // printf("\nInside Consumer Process --> Consumer with process number (assigned as consumer id) %d received a PRODUCE request message, hence did nothing to the message and pushed the message back to queue\n", CT_no);
                            sem_post(Queue_full);
                            sem_post(binary_semaphore_lock); 

                        }
                        else
                        {
                            //Write operation failed

                            sem_post(binary_semaphore_lock);
   
                            printf("\nInside Consumer Process --> Consumer with process number %d unable to access Queue\n", CT_no);
                        }
                    }
                }
                else
                {
                    //The Consumer process enters here when mesg.consumer_id != CT_no
                    //i.e., when the read message has a different consumer_id variable that the consumer id
                    //of the current process. This means that this message is meant for a different consumer process.
                    //Then the consumer process will write this message back onto Queue without making any changes to it
                    
                    //Checking if there is atleast 1 empty_slot before writing the message
                    // sem_wait(Queue_empty);

                    //Attempting to write
                    sem_wait(Queue_empty);
                    sem_wait(binary_semaphore_lock);
                    ssize_t conWrite = write(Queue, &mesg, sizeof(struct message));
                    if ( conWrite > -1)
                    {
                        //If conWrite > -1 then the write operation was successfull
                        //Then we increment Queue_full by 1 and release the binary_semaphore_lock

                        // printf("\nInside Consumer Process --> Consumer with process number (assigned as consumer id) %d received an ACKNOWLEDGEMENT MESSAGE with different consumer id %d, hence didn't consume the data and pushed the message back to queue\n", CT_no,mesg.consumer_id);
                        sem_post(Queue_full);
                        sem_post(binary_semaphore_lock);
                        // printf("\nleft lock %d\n", ct_no);
                    }
                    else
                    {
                        //The consumer process enters here if the write operation has failed

                        sem_post(binary_semaphore_lock); 
                        //printf("\nleft lock %d\n", ct_no);
                        printf("\nInside Consumer Process --> Consumer with process number %d unable to access Queue\n", CT_no);
                    }
                }
            }
            else
            {
                //The consumer process enters here if the initial read operation has failed

                sem_post(binary_semaphore_lock);
                
                printf("\nInside Consumer Process --> Consumer with process number %d failed to read from Queue\n", CT_no);
            }
        }
        
    }
    
    //If outside the while(True) loop then then the consumer process is beginning to exit

    printf("\nInside Consumer Process --> Consumer with process number %d exiting\n", CT_no);

    //We increment the semaphore num_exited_consumers (indicating the total number of consumer
    //processes which have exited) by 1 as one consumer will exit now
    sem_post(num_exited_consumers);
    int num_exited_cons;
    // printf("consumers here");
    sem_getvalue(num_exited_consumers, &num_exited_cons);

    //storing the total number of exited consumers in num_exited_cons
    printf("\nNumber of consumers exited %d\n", num_exited_cons);
    if (num_exited_cons==CONSUMERS_COUNT){
        
        //if num_exited_cons==CONSUMERS_COUNT that means all the other consumer processes have exited and
        //this is the last consumer process to exit
        //then we insert a message from this last consumer process to the queue
        //with the message status variable set to STOPPING_CRITERIA_STATUS which will
        //indicate to the producer processes that all the consumer processes have exited when the producer process will
        //read the data
        //then the producer processes will start exiting

        // printf ("here");
        struct message mesg;

        //setting message status variable to STOPPING_CRITERIA_STATUS
        mesg.status=STOPPING_CRITERIA_STATUS;


        //Waiting for the Queue to have atleast 1 empty slot
	    sem_wait(Queue_empty);

        //waiting for the binary_semaphore_lock
        sem_wait(binary_semaphore_lock);

        //Writing this termination message to Queue
	    write(Queue, &mesg, sizeof(struct message));

        //Incrementing Queue_full by 1 as one message was written to Queue
	    sem_post(Queue_full);

        //Releasing the binary_semaphore_lock
        sem_post(binary_semaphore_lock);
        // printf ("here 2");

        //closing the Queue
        close(Queue);

        //Exiting
        exit(EXIT_SUCCESS);
    }
    else{

        //The consumer process is here if this is not the last process 
        //then we are just closing the queue and exiting
        close(Queue);
        exit(EXIT_SUCCESS);
    }
}



//***************THE MAIN FUNCTION*********************//


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

    //Printing the semaphore values
    printf("\nValues of binary_semaphore_lock = %d, Queue_empty = %d, Queue_full = %d\n", a,b,c );

    //getting time of the day and storing it as starting_time
    gettimeofday(&starting_time, NULL);

    //declaring the producer and consumer threads
    pid_t producer_id[PRODUCERS_COUNT], consumer_id[CONSUMERS_COUNT];

    //declaring the FIFO Queue
    const char *Queue = FIFO_QUEUE;

    //creating the Fifo queue
    mkfifo(Queue, 0666);

    int i, j, k;

    //Spawning Child Processes for Producers

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

    //Spawning Child Processes for Consumers

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


    //The main function enters here when all the child processes have terminated

    printf("\nAll producer and consumer processes terminated\n");
    printf("\nExiting the program\n");
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