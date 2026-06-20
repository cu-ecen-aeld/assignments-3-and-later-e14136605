#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
// 1. 將傳入的指標轉型回我們定義的結構體
	struct thread_data* thread_func_args = (struct thread_data *) thread_param;

	// 2. 取得鎖之前先等待指定的時間 (毫秒轉微秒要乘 1000)
	usleep(thread_func_args->wait_to_obtain_ms * 1000);

	// 3. 取得互斥鎖 (Lock)
	pthread_mutex_lock(thread_func_args->mutex);

	// 4. 取得鎖之後，等待指定的時間再釋放
	usleep(thread_func_args->wait_to_release_ms * 1000);

	// 5. 釋放互斥鎖 (Unlock)
	pthread_mutex_unlock(thread_func_args->mutex);

	// 6. 將執行結果標記為成功
	thread_func_args->thread_complete_success = true;

	// 7. 回傳原本的指標
	return thread_param;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
// 1. 動態分配記憶體給 thread_data 結構體
	struct thread_data* data = (struct thread_data*) malloc(sizeof(struct thread_data));
	if (data == NULL) {
    	return false; // 如果記憶體分配失敗，回傳 false
	}

	// 2. 將傳入的參數填入結構體中
	data->mutex = mutex;
	data->wait_to_obtain_ms = wait_to_obtain_ms;
	data->wait_to_release_ms = wait_to_release_ms;
	data->thread_complete_success = false;

	// 3. 建立並啟動執行緒
	int rc = pthread_create(thread, NULL, threadfunc, data);
    
	// 4. 檢查執行緒是否建立成功
	if (rc != 0) {
    	free(data);   // 如果失敗，記得把剛剛分配的記憶體還給系統
    	return false;
	}

	return true;

    return false;
}

