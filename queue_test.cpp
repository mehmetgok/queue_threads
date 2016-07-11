// queue::push/pop
// http://www.cplusplus.com/reference/queue/queue/pop/

#include <iostream>       // std::cin, std::cout
#include <queue>          // std::queue

#include <stdint.h>

using namespace std;


int main ()
{
	queue<int> myqueue;
	queue<int16_t*> adcDataQueue;

	int k = 100;

	int16_t *adcData1 = new int16_t[k];
	int16_t *adcData2 = new int16_t[k];

	int16_t *gotData;

	for (int i=0;i<k;i++)
	adcData1[i] = i;


	for (int i=0;i<k;i++)
	adcData2[i] = k-i;

	adcDataQueue.push(adcData1);
	adcDataQueue.push(adcData2);

	gotData = adcDataQueue.front();

	for (int i=0;i<k;i++)
	cout << ' ' << gotData[i];

	adcDataQueue.pop();

	cout << endl;

	gotData = adcDataQueue.front();

	for (int i=0;i<k;i++)
	cout << ' ' << gotData[i];

	adcDataQueue.pop();	
	
	cout << endl << "Size of queue:" << adcDataQueue.size() << endl;


	delete adcData1;
	delete adcData2;

	return 0;
}
