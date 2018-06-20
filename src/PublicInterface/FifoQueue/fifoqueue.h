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


//队列接口类 
class IBaseQueue
{
public:
	virtual ~IBaseQueue(){}

public:
	virtual bool  PushToQueue(void *packet, int len) = 0;
	//带超时等待机制的take
	virtual int32_t TakeFromQueue(void *packet, int& len, int waittime = 200){ return 0; };//默认实现
	virtual int32_t TakeFromQueue(void *packet, int& len, bool erase){ return 0; };//默认实现

	virtual void	ClearQueue() = 0;
	virtual	bool 	QueueIsEmpty() = 0;

};


class DynFifoQueue
{

public:

	DynFifoQueue(int fifo_deep, int data_deep);
	~DynFifoQueue();

public:

	virtual bool  PushToQueue(void *packet, int len);
	virtual int32_t TakeFromQueue(void *packet, int& len, int waittime = 200);

	virtual void	ClearQueue();
	virtual	bool 	QueueIsEmpty();


private:

	volatile  uint32_t fifo_counter;
	ILock	*queuelock;
	ISem	*queuesem;
	unsigned int p_fifo_deep;
	unsigned int p_data_deep;
	dynamic_fifoqueue_t *ptr_fifo;
	std::list<dynamic_fifoqueue_t *>  	m_dyn_list;

};
class FifoQueue : public IBaseQueue
{
	
	public:
		FifoQueue();
		~FifoQueue();

	public:
		virtual bool  PushToQueue(void *packet, int len);
		virtual int32_t TakeFromQueue(void *packet, int& len, int waittime = 200);

		virtual void	ClearQueue();
		virtual	bool 	QueueIsEmpty();

	private:

		std::list<fifoqueue_t *>  	m_list;
		fifoqueue_t fifobuff[FIFODEEP];
		volatile  uint32_t fifo_counter;
		ILock	*queuelock;
		ISem	*queuesem;

	
};



//高效队列，强烈建议使用以下两种队列接口。
class RingQueue : public IBaseQueue
{

public:
	RingQueue();
	~RingQueue();

public:
	virtual bool  PushToQueue(void *packet, int len);
	virtual int32_t TakeFromQueue(void *packet, int& len, bool erase);

	virtual void	ClearQueue();
	virtual	bool 	QueueIsEmpty();

private:

	fifoqueue_t ringqueue[FIFODEEP];
	int queue_head;
	int queue_tail;
	ILock	*queuelock;
};
class DynRingQueue : public IBaseQueue
{
public:
	DynRingQueue(int ring_deep, int data_deep);
	~DynRingQueue();

public:
	virtual bool  PushToQueue(void *packet, int len);
	virtual int32_t TakeFromQueue(void *packet, int& len, bool erase);

	virtual void	ClearQueue();
	virtual	bool 	QueueIsEmpty();

private:
	
	ILock	*queuelock;
	int queue_head;
	int queue_tail;
	unsigned int p_ring_deep;
	unsigned int p_data_deep;
	dynamic_fifoqueue_t *ptr_ring;

};




#endif