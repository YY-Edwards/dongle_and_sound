/*
 * FifoQueue.c
 *
 * Created: 2016/12/19
 * Author: EDWARDS
 */ 
#include "fifoqueue.h"

DynFifoQueue::DynFifoQueue(int fifo_deep, int data_deep)
:fifo_counter(0)
, queuelock(NULL)
, queuesem(NULL)
, ptr_fifo(NULL)
{

#ifdef WIN32
	queuelock = new CriSection();
#else
	queuelock = new Mutex((const char *)"dynqueuelocker");
#endif

	queuesem = new MySynSem();


	p_fifo_deep = fifo_deep;
	p_data_deep = data_deep;
	ptr_fifo = new dynamic_fifoqueue_t[fifo_deep];//分配队列深度内存
	for (int i = 0; i < fifo_deep; i++)
	{
		(ptr_fifo + i)->data = new char[data_deep];//分配队列宽度内存
		memset((ptr_fifo + i)->data, 0x00, data_deep);//清零
		(ptr_fifo + i)->len = 0;
	}

	log_debug("dynamic queue created\n");
}

DynFifoQueue::~DynFifoQueue()
{
	ClearDynQueue();//清空队列
	if (queuelock != NULL)
	{
		delete queuelock;
		queuelock = NULL;
	}
	if (queuesem != NULL)
	{
		delete queuesem;
		queuesem = NULL;
	}

	if (ptr_fifo != NULL)//是否释放动态内存
	{
		for (unsigned int i = 0; i < p_fifo_deep; i++)
		{
			if (((ptr_fifo + i)->data) != NULL)
			{
				delete[]((ptr_fifo + i)->data);
				(ptr_fifo + i)->data = NULL;
			}
		}

		delete[]ptr_fifo;
		ptr_fifo = NULL;
	}
	log_debug("delete:dynamic queue\n");

}


void DynFifoQueue::ClearDynQueue()
{
	queuelock->Lock();
	m_dyn_list.clear();
	queuelock->Unlock();

}

bool DynFifoQueue::PushToDynQueue(void *packet, unsigned int len)
{
	if (ptr_fifo == NULL)
	{
		log_warning("ptr_fifo == NULL\n");
		return false;//no support
	}

	if (len>p_data_deep)return false;//data overout
	if (m_dyn_list.size() >= p_fifo_deep)return false;//fifo full

	queuelock->Lock();

	memcpy(ptr_fifo[fifo_counter].data, packet, len);

	ptr_fifo[fifo_counter].len = len;
	m_dyn_list.push_back((ptr_fifo + fifo_counter));//将一个结构体数据地址插入链表尾
	fifo_counter++;
	if (fifo_counter >= p_fifo_deep){
		fifo_counter = 0;
	}

	queuelock->Unlock();

	queuesem->SemPost();

	return true;

}

int32_t DynFifoQueue::TakeFromDynQueue(void *packet, unsigned int& len, int waittime)
{

	dynamic_fifoqueue_t *sBuffer = NULL;

	int ret = queuesem->SemWait(waittime);
	if ((ret < 0) || (ret >= 1))
	{
		return ret;//timeout or failed
	}
	if (!m_dyn_list.empty())
	{
		queuelock->Lock();

		sBuffer = m_dyn_list.front();//返回链表第一个结构体数据包地址给sbuffer
		m_dyn_list.pop_front();//并删除链表第一个数据包地址
		memcpy(packet, sBuffer->data, sBuffer->len);
		len = sBuffer->len;

		queuelock->Unlock();
		return ret;
	}
	else
	{
		return -2;//no happen but must check

	}


}


FifoQueue::FifoQueue()
:fifo_counter(0)
, queuelock(NULL)
, queuesem(NULL)
{

#ifdef WIN32
	queuelock = new CriSection();
#else
	queuelock = new Mutex((const char *)"queuelocker");
#endif

	queuesem = new MySynSem();

	for (int i = 0; i < FIFODEEP; i++){
		memset(&(fifobuff[i].data), 0x00, DATADEEP);
		memset(&(fifobuff[i].len), 0x00, sizeof(uint8_t));
	}

}

FifoQueue::~FifoQueue()
{
	ClearQueue();//清空队列
	if (queuelock != NULL)
	{
		delete queuelock;
		queuelock = NULL;
	}
	if (queuesem != NULL)
	{
		delete queuesem;
		queuesem = NULL;
	}
	
}

void FifoQueue::ClearQueue()
{
	queuelock->Lock();
	m_list.clear();
	queuelock->Unlock();

}

bool  FifoQueue::QueueIsEmpty()
{
	return(m_list.empty());
}

bool FifoQueue::PushToQueue(void *packet, int len)
{
	//int err = 0;
	if (len>DATADEEP)return false;//data overout
	if (m_list.size() >= FIFODEEP)return false;//fifo full

	queuelock->Lock();
	////清空结构体
	//memset(&(fifobuff[fifo_counter].data), 0x00, DATADEEP);
	//memset(&(fifobuff[fifo_counter].len), 0x00, sizeof(uint8_t));
	//memcpy(&fifobuff[fifo_counter][0], packet, len);
	//m_list.push_back(&fifobuff[fifo_counter][0]);//将一个数据包地址插入链表尾

	memcpy(&fifobuff[fifo_counter].data, packet, len);
	fifobuff[fifo_counter].len = len;
	m_list.push_back(&fifobuff[fifo_counter]);//将一个结构体数据地址插入链表尾
	fifo_counter++;
	if (fifo_counter > 19){
		fifo_counter = 0;
	}

	queuelock->Unlock();

	queuesem->SemPost();

	return true;

}

int32_t FifoQueue::TakeFromQueue(void *packet, int& len, int waittime)
{

	//char* sBuffer = NULL;
	fifoqueue_t *sBuffer = NULL;

	int ret = queuesem->SemWait(waittime);
	if ((ret < 0) || (ret >= 1))
	{
		return ret;//timeout or failed
	}
	if (!m_list.empty())
	{
			queuelock->Lock();

			sBuffer = m_list.front();//返回链表第一个结构体数据包地址给sbuffer
			m_list.pop_front();//并删除链表第一个数据包地址
			memcpy(packet, sBuffer->data, sBuffer->len);
			len = sBuffer->len;

			queuelock->Unlock();
			return ret;
	}
	else
	{
		return -2;//no happen but must check

	}


}



