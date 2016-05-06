#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h> /* Ctrl+C Handling */
#include <unistd.h>
#include <stdint.h> /* Extra types */

#include <queue>          /* std::queue */

/* Linux RT */
#include <sched.h>
#include <sys/mman.h>

#include <string.h>

using namespace std;


#define STEP_LIMIT 1000
#define SLEEP_US 500

#define BILLION  1000000000L


// Producing times
queue<uint64_t> fifoProdTimes;

// Consuming time
queue<uint64_t> fifoConsTimes;

// ADC Data Queue
queue<int16_t*> adcDataQueue;


static volatile int keepRunning = 1;

void intrHandler(int dummy) {
	keepRunning = 0;
}

pthread_t thDataGetter, thDataProcessor;

pthread_mutex_t lock;



#define MY_PRIORITY_1 (49) /* we use 49 as the PRREMPT_RT use 50
                            as the priority of kernel tasklets
                            and interrupt handler by default */
                            
                            
#define MY_PRIORITY_2 (45) /* we use 48 as the PRREMPT_RT use 50
                            as the priority of kernel tasklets
                            and interrupt handler by default */

#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */

#define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */

void stack_prefault(void) {

        unsigned char dummy[MAX_SAFE_STACK];

        memset(dummy, 0, MAX_SAFE_STACK);
        return;
}





void *dataGetter(void*);
void *dataProcessor(void*);


int main(void) {
	
	uint64_t file_value1, file_value2;
	
	FILE * pFileProdTimes, *pFileConsTimes, *pFileDifTimes;

	
	pFileProdTimes = fopen ("prodTimes.txt","w");
	pFileConsTimes = fopen ("consTimes.txt","w");
	pFileDifTimes = fopen ("difTimes.txt","w");
	
	 /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-2);
    }
	
	
	signal(SIGINT, intrHandler);
	
	pthread_mutex_lock(&lock);
	
	pthread_create(&thDataProcessor, NULL,  &dataProcessor, NULL);
	pthread_create(&thDataGetter, NULL, &dataGetter, NULL);
	
	pthread_mutex_unlock(&lock);
	
	
	pthread_join( thDataGetter, NULL);	
	pthread_join( thDataProcessor, NULL);
	
	while (fifoProdTimes.size()>0) {
		file_value1 = fifoProdTimes.front();
		file_value2 = fifoConsTimes.front();
		
		fprintf (pFileProdTimes, "%llu\r\n", file_value1);
		fprintf (pFileConsTimes, "%llu\r\n", file_value2);
		fprintf (pFileDifTimes, "%llu\r\n", file_value2-file_value1);
				
		fifoProdTimes.pop();	
		fifoConsTimes.pop();	
	}
	
	
	fclose(pFileProdTimes);
	fclose(pFileConsTimes);
	fclose(pFileDifTimes);
	
	
	return 0;
}

void *dataGetter(void*) {
	
	
	int counts = 1;
	// struct timeval prodTime;
	
	struct timespec prodTime;
	
	struct timespec t;
	
	
	struct sched_param param;
	
	
	int interval = 900000; /* 900 us*/
		
	uint64_t prod_time;
	int16_t* adcData;
		
	
	/* Declare ourself as a real time task */
    param.sched_priority = MY_PRIORITY_1;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler failed");
        exit(-1);
    }
		
        
    clock_gettime(CLOCK_MONOTONIC ,&t);
	
    /* start after 900 usecs */
    t.tv_nsec += interval;
	
	while (keepRunning) {
		
		/* wait until next shot */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		
		
		// Add value to the queue		
		pthread_mutex_lock(&lock);
		
			adcData = new int16_t;
			
			*adcData = (int16_t)counts;
		
			adcDataQueue.push(adcData);
			
			// Get the time	
			// gettimeofday(&prodTime, NULL);
			
			// Convert time to usecs
			// prod_time = (prodTime.tv_sec * (uint64_t)1000000) + (prodTime.tv_usec);
			
			
			// Get real time clock
			clock_gettime( CLOCK_REALTIME, &prodTime);	
			
			// convert to micro seconds			
			prod_time = (prodTime.tv_sec*(uint64_t)BILLION + (uint64_t)prodTime.tv_nsec)/1000;			
			
			fifoProdTimes.push(prod_time);
	
			counts++;
	
		pthread_mutex_unlock(&lock);
		
		
		
		// usleep(SLEEP_US);
		
		/* calculate next shot */
        t.tv_nsec += interval;

        while (t.tv_nsec >= NSEC_PER_SEC) {
                t.tv_nsec -= NSEC_PER_SEC;
                t.tv_sec++;
        }
		
		
		
		
		if (counts>STEP_LIMIT)
			break;
	}
	
}

void *dataProcessor(void*) {
		
	
	
	int counts = 1;
	// struct timeval consTime;
	
	struct timespec consTime;
	struct timespec t;
	
	
	struct sched_param param;
	
	int interval = 700000; /* 700 us*/
	
	uint64_t cons_time;
	
	// Data taken from dataFifo
	uint64_t count_data;
	
	// Data taken from dataFifo
	int16_t* adcData;
		
	
	/* Declare ourself as a real time task */

        param.sched_priority = MY_PRIORITY_2;
        if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
                perror("sched_setscheduler failed");
                exit(-1);
        }
		
	

      
	
	   clock_gettime(CLOCK_MONOTONIC ,&t);
        /* start after 500 usecs */
          t.tv_nsec += interval;
	
	
	
	
	while (keepRunning) {
		
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		 
		
		
		// Get data from queue
		pthread_mutex_lock(&lock);
			
			if (adcDataQueue.size()>0) 
			{
				
				adcData = adcDataQueue.front();
						
				// gettimeofday(&consTime, NULL);
			
				// cons_time = (consTime.tv_sec * (uint64_t)1000000) + (consTime.tv_usec);
				
				// Get realtime clock
				clock_gettime( CLOCK_REALTIME, &consTime);			
			
				// convert to micro seconds			
				cons_time = (consTime.tv_sec*(uint64_t)BILLION + (uint64_t)consTime.tv_nsec)/1000;
				
					
				fifoConsTimes.push(cons_time);
			
				counts++;	

				adcDataQueue.pop();				
				
				delete adcData;
			
			}
			else
			{
				printf("No Data available in the queue\r\n");			
			}
		
		
		pthread_mutex_unlock(&lock);
		
		// usleep(SLEEP_US);
		
		 /* calculate next shot */
                t.tv_nsec += interval;

                while (t.tv_nsec >= NSEC_PER_SEC) {
                       t.tv_nsec -= NSEC_PER_SEC;
                        t.tv_sec++;
                }
		
		
		
		
		if (counts>STEP_LIMIT)
		break;
		
	}
}
