#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h> /* Ctrl+C Handling */
#include <unistd.h>
#include <stdint.h> /* Extra types */

#include <queue>          /* std::queue */

using namespace std;


#define STEP_LIMIT 1000
#define SLEEP_US 500


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

void *dataGetter(void*);
void *dataProcessor(void*);


int main(void) {
	
	
	uint64_t file_value;
	
	
	FILE * pFileProdTimes, *pFileConsTimes;
	
	pFileProdTimes = fopen ("prodTimes.txt","w");
	pFileConsTimes = fopen ("consTimes.txt","w");
	
	
	signal(SIGINT, intrHandler);
	
	pthread_create(&thDataProcessor, NULL,  &dataProcessor, NULL);
	pthread_create(&thDataGetter, NULL, &dataGetter, NULL);
	
	pthread_join( thDataGetter, NULL);	
	pthread_join( thDataProcessor, NULL);
	
	while (fifoProdTimes.size()>0)
	{
		file_value = fifoProdTimes.front();
		
		fprintf (pFileProdTimes, "%lu\r\n", file_value);
		
		fifoProdTimes.pop();	
	}
	
	while (fifoConsTimes.size()>0)
	{
		file_value = fifoConsTimes.front();
		
		fprintf (pFileConsTimes, "%lu\r\n", file_value);
		
		fifoConsTimes.pop();	
	}
	
	
	fclose(pFileProdTimes);
	fclose(pFileConsTimes);
	
	
	return 0;
}

void *dataGetter(void*) {
	
	int counts = 1;
	struct timeval prodTime;
	
	uint64_t prod_time;
	
	int16_t* adcData;

	
	
	while (keepRunning) {
		
		
		// Add value to the queue		
		pthread_mutex_lock(&lock);
		
			adcData = new int16_t;
			
			*adcData = (int16_t)counts;
		
			adcDataQueue.push(adcData);
			
			// Get the time	
			gettimeofday(&prodTime, NULL);
			
			// Convert time to usecs
			prod_time = (prodTime.tv_sec * (uint64_t)1000000) + (prodTime.tv_usec);
			
			fifoProdTimes.push(prod_time);
	
			counts++;
	
		pthread_mutex_unlock(&lock);
		
		
		
		usleep(SLEEP_US);
		
		
		if (counts>STEP_LIMIT)
			break;
	}
	
}

void *dataProcessor(void*) {
	
	
	int counts = 1;
	struct timeval consTime;
	
	uint64_t cons_time;
	
	// Data taken from dataFifo
	int16_t* adcData;
	
	while (keepRunning) {
		
		
		// Get data from queue
		pthread_mutex_lock(&lock);
			
			if (adcDataQueue.size()>0) 
			{
				
				adcData = adcDataQueue.front();
						
				gettimeofday(&consTime, NULL);
			
				cons_time = (consTime.tv_sec * (uint64_t)1000000) + (consTime.tv_usec);
					
				fifoConsTimes.push(cons_time);
			
				counts++;	

				adcDataQueue.pop();				
				
				delete adcData;
			
			}
			else
			{
				printf("No Data available int the queue\r\n");			
			}
		
		
		pthread_mutex_unlock(&lock);
		
		usleep(SLEEP_US);
		
		
		
		if (counts>STEP_LIMIT)
		break;
		
	}
}
