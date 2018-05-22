// This is the main DLL file.

#include "stdafx.h"
#include "aS_lib.h"

using namespace aS;
	
aS::InstanceManager::InstanceManager()
{
	this->TimeOut = AS_IM_TIMEOUT;
	this->VectorMutex = CreateSemaphore(NULL,1 ,1 ,NULL);
}

aS::InstanceManager::InstanceManager(u32 TimeOut)
{
	this->TimeOut = TimeOut;
	this->VectorMutex = CreateSemaphore(NULL,1 ,1 ,NULL);
}

aS::InstanceManager::~InstanceManager()
{
	CloseHandle(this->VectorMutex);
	this->VectorMutex = NULL;
}
		
i32 aS::InstanceManager::AddInstance(void *InstanceHandle)
{
	i32 dwWaitResult;
		
	if (this->VectorMutex != NULL)
	{
		dwWaitResult = WaitForSingleObject(this->VectorMutex, this->TimeOut);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			return AS_MUTEX_NOT_LONGER_VALID;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			return AS_WAIT_TIMEOUT;
		}

		IM_HANDLE_STRUCT temp;
		temp.Handle = InstanceHandle;
		temp.Status = CreateSemaphore(NULL, 1, 1, NULL);
		this->HandleVector.push_back(temp);

		ReleaseSemaphore(this->VectorMutex, 1, NULL);		
	}
	else
	{
		return AS_INVALID_MUTEX;
	}
	return AS_NO_ERROR;
}

i32 aS::InstanceManager::SetInstanceBusy(void *InstanceHandle)
{
	i32			dwWaitResult = 0;
	u32			Position = 0;
	i32			iResult = 0;
		
	if (this->VectorMutex != NULL)
	{
		dwWaitResult = WaitForSingleObject(this->VectorMutex, this->TimeOut);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			return AS_MUTEX_NOT_LONGER_VALID;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			return AS_WAIT_TIMEOUT;
		}
	}
	else
	{
		return AS_INVALID_MUTEX;
	}
		
	iResult = this->FindInstance(InstanceHandle, &Position);
			
	if (iResult == AS_NO_ERROR)
	{
		dwWaitResult = WaitForSingleObject(this->HandleVector[Position].Status, this->TimeOut);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			iResult = AS_MUTEX_NOT_LONGER_VALID;
			break;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			iResult = AS_INSTANCE_BUSY;
			break;
		}
	}

	ReleaseSemaphore(this->VectorMutex, 1, NULL);

	return iResult;
}

i32 aS::InstanceManager::SetInstanceReady(void *InstanceHandle)
{
	i32			dwWaitResult = 0;
	u32			Position = 0;
	i32			iResult = 0;
		
	if (this->VectorMutex != NULL)
	{
		dwWaitResult = WaitForSingleObject(this->VectorMutex, this->TimeOut);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			return AS_MUTEX_NOT_LONGER_VALID;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			return AS_WAIT_TIMEOUT;
		}
	}
	else
	{
		return AS_INVALID_MUTEX;
	}
		
	iResult = this->FindInstance(InstanceHandle, &Position);
			
	if (iResult == AS_NO_ERROR)
	{
		dwWaitResult = WaitForSingleObject(this->HandleVector[Position].Status, 0);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			iResult = AS_MUTEX_NOT_LONGER_VALID;
			break;
		case WAIT_OBJECT_0:
			iResult = AS_INSTANCE_READY;
		case WAIT_TIMEOUT:
			ReleaseSemaphore(this->HandleVector[Position].Status, 1, NULL);
			break;
		}
	}

	ReleaseSemaphore(this->VectorMutex, 1, NULL);

	return iResult;
}

i32 aS::InstanceManager::RemInstance(void *InstanceHandle)
{
	i32			dwWaitResult = 0;
	u32			Position = 0;
	i32			iResult = 0;
		
	if (this->VectorMutex != NULL)
	{
		dwWaitResult = WaitForSingleObject(this->VectorMutex, this->TimeOut);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			return AS_MUTEX_NOT_LONGER_VALID;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			return AS_WAIT_TIMEOUT;
		}
	}
	else
	{
		return AS_INVALID_MUTEX;
	}
		
	iResult = this->FindInstance(InstanceHandle, &Position);
			
	if (iResult == AS_NO_ERROR)
	{
		dwWaitResult = WaitForSingleObject(this->HandleVector[Position].Status, this->TimeOut);
			
		switch(dwWaitResult)
		{
		case WAIT_ABANDONED:
			iResult = AS_MUTEX_NOT_LONGER_VALID;
			break;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			iResult = AS_INSTANCE_BUSY;
			break;
		}
	}

	if (iResult == AS_NO_ERROR)
	{
		CloseHandle(this->HandleVector[Position].Status);
		this->HandleVector.erase(HandleVector.begin()+Position);
	}		

	ReleaseSemaphore(this->VectorMutex, 1, NULL);

	return iResult;
}

i32 aS::InstanceManager::FindInstance(void *InstanceHandle, u32 *Position)
{
	u32		 i = 0;
	while ((i < this->HandleVector.size()) && (this->HandleVector[i].Handle != InstanceHandle))
	{
		i++;
	}

	if (i == this->HandleVector.size())
	{
		*Position = 0;
		return AS_UNKNOWN_INSTANCE;
	}
		
	*Position = i;

	return AS_NO_ERROR;
}