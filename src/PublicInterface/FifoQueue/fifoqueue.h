/*
 * FifoQueue.h
 *
 * Created: 2018/01/03
 * Author: EDWARDS
 */ 

#ifndef fifoqueue_h_
#define fifoqueue_h_
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <list>
#include <queue>
#include <stdint.h>
#include "syninterface.h"
#include "log.h"

#define FIFODEEP  30
#define DATADEEP  1500
#pragma pack(push, 1)
typedef struct{
	char	data[DATADEEP];
	int		len;

}fifoqueue_t;
typedef struct{
	
	char	*data;
	int		len;

}dynamic_fifoqueue_t;

#pragma pack(pop)


class DynFifoQueue
{

public:

	DynFifoQueue(int fifo_deep, int data_deep);
	~DynFifoQueue();

public:
	bool  			PushToDynQueue(void *packet, unsigned int len);
	int32_t 		TakeFromDynQueue(void *packet, unsigned int& len, int waittime = 200);
	void			ClearDynQueue();


private:

	volatile  uint32_t fifo_counter;
	ILock	*queuelock;
	ISem	*queuesem;
	unsigned int p_fifo_deep;
	unsigned int p_data_deep;
	dynamic_fifoqueue_t *ptr_fifo;
	std::list<dynamic_fifoqueue_t *>  	m_dyn_list;

};


class FifoQueue 
{
	
	public:
		FifoQueue();
		~FifoQueue();

	public:
		bool  			PushToQueue(void *packet, int len);
		int32_t 		TakeFromQueue(void *packet, int& len, int waittime =200);

		void			ClearQueue();
		bool 			QueueIsEmpty();

	private:

		std::list<fifoqueue_t *>  	m_list;
		fifoqueue_t fifobuff[FIFODEEP];
		volatile  uint32_t fifo_counter;
		ILock	*queuelock;
		ISem	*queuesem;

	
};



#endif