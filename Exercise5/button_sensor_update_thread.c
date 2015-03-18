#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "elevator_model_data_structure.h"
#include "elev.h"
int push_task(const int floor);

void * keyboard_read_thread(){
//	FILE * keyin = fopen("./keyboard_in", "r" );
//	FILE * keyout = fopen("./keyboard_out", "w");
	while(1){
		int read;
		printf(  "\nInput floor: ");
		scanf( "%d", &read);
		printf( "\nDesired floor is: %d",read);
		//set_desired_floor(read);
		push_task(read);
		//sleep(1);
	}
	return NULL;
}
#define  EMPTY_TASK 0XFFFF
int task_queue[N_FLOORS] = {EMPTY_TASK,EMPTY_TASK,EMPTY_TASK,EMPTY_TASK};
volatile int task_queue_top = 0;
int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}
pthread_mutex_t task_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_task_cv = PTHREAD_COND_INITIALIZER;
event_t new_task_event = {
		.cv = &new_task_cv,
		.mutex = &task_queue_lock
};
int fetch_task(void){
	static int current_fetch_head = 0;/* either top or 0 to the sorted queue */

	if(task_queue_top>=0&&task_queue_top<N_FLOORS){
		if(abs(task_queue[task_queue_top]-get_last_stable_floor()-get_motor_moving_vector())
				>=
				abs(task_queue[0]-get_last_stable_floor()-get_motor_moving_vector())){
			/* moving up fetch from the smallest */
			current_fetch_head = 0;
		} else {
			current_fetch_head = task_queue_top;
		}
		printf("fetch head is %d\n\n",current_fetch_head);
		if(task_queue_top!=0)
			task_queue_top--;
		int ret = task_queue[current_fetch_head];
		task_queue[current_fetch_head] = EMPTY_TASK;
		qsort(task_queue, N_FLOORS, sizeof(int), cmpfunc);
		return ret;
	}
	return EMPTY_TASK;
}
int fetch_task_max(void){
	return task_queue[task_queue_top];
}
int fetch_task_min(void){
	return task_queue[0];
}
int push_task(const int floor){
	if(floor<0 || floor>=N_FLOORS)
		return 0;
	//pthread_mutex_lock(new_task_event.mutex);
	for (int i = 0; i < N_FLOORS; ++i) {
		if(floor == task_queue[i]){
			//pthread_cond_signal(new_task_event.cv);
			//pthread_mutex_unlock(new_task_event.mutex);
			return 0; /*rejected*/
		}
	}
	if(task_queue_top<0 || task_queue_top>= N_FLOORS){
		puts("error in task_queue_top");
		return 0;
	}
	if(task_queue_top != N_FLOORS-1)
			task_queue_top ++;

	task_queue[task_queue_top] = floor;
	qsort(task_queue, N_FLOORS, sizeof(int), cmpfunc);

		//pthread_cond_signal(new_task_event.cv);
	//pthread_mutex_unlock(new_task_event.mutex);

	return 1;
}

int IsRequestAccepted(elev_button_type_t type, int floor){
	/* when motor is free then accepted, change desired floor */
//	if(get_motor_moving_vector())
//		return 0;
	int ret = push_task(floor);
	return ret;
}
pthread_t keyboard_read_th, stop_button_controller_th, elevator_running_th;
void * stop_button_controller_thread(void * data){
	while(1)
	{
		pthread_mutex_lock(input_event_ptr->mutex);
		pthread_cond_wait(input_event_ptr->cv, input_event_ptr->mutex);
		input_status_t read = get_input_status_unsafe();
		light_status_t write = get_light_status();
		if(read.stop_button){
			write.stop_light = !write.stop_light;
			set_light_status(write);
			if(write.stop_light){
				//set_desired_floor(MOTOR_EM_STOP_CMD);
			}
		}
		pthread_mutex_unlock(input_event_ptr->mutex);
	}

	return NULL;
}
void go_to_desired_floor(int floor){
	while(floor != get_input_status_unsafe().floor_sensor){
		pthread_mutex_lock(floor_reached_event_ptr->mutex);
		printf("go to desired floor %d\n\n",floor);
		set_desired_floor_unsafe(floor);
		pthread_cond_wait(floor_reached_event_ptr->cv, floor_reached_event_ptr->mutex);
		pthread_mutex_unlock(floor_reached_event_ptr->mutex);
	}
}
void open_wait_close(void){
	light_status_t light = get_light_status();
	light.door_open_light = 1;
	set_light_status(light);
	sleep(2);
	input_status_t input = get_input_status();
	while(input.obst_button){
		sleep(1);
		input = get_input_status();
	}
	light = get_light_status();
	light.door_open_light = 0;
	set_light_status(light);
	sleep(2);
}

void *elevator_running_process(void * data){
	while(1){
		/**
		 *  fetch a new task
		 */
		int floor = EMPTY_TASK;
		printf("empty task init!\n\n");
		while(floor == EMPTY_TASK){
			usleep(50000);
			floor = fetch_task();
			printf("fetch task %d\n\n\n", floor);
		}

		printf("calling go to floor, wait for reached!\n\n");
		go_to_desired_floor(floor);
		printf("reached floor, recieved reached signal!\n\n");
		sleep(1); /* wait for stop stable then open */
		/* clear the correspondent floor light */
		light_status_t light = get_light_status();
		light.floor_button_lights[floor][BUTTON_COMMAND] = 0;
		if(get_motor_last_none_zero_motor_moving_vector()>0 || floor == fetch_task_max()){
			light.floor_button_lights[floor][BUTTON_CALL_UP] = 0;
		}else if (get_motor_last_none_zero_motor_moving_vector()>0 || floor == fetch_task_max()){
			light.floor_button_lights[floor][BUTTON_CALL_DOWN] = 0;
		}else {
			light.floor_button_lights[floor][BUTTON_CALL_UP] = 0;
			light.floor_button_lights[floor][BUTTON_CALL_DOWN] = 0;
		}
		set_light_status(light);
			open_wait_close();


	}
	return NULL;
}
int main(){

	elevator_model_init(NULL);
	int rc = pthread_create(&keyboard_read_th, NULL, keyboard_read_thread, NULL);
	if (rc){
		perror("pthread_create");
		exit(-1);
	}
	rc = pthread_create(&stop_button_controller_th, NULL, stop_button_controller_thread, NULL);
	if (rc){
		perror("pthread_create");
		exit(-1);
	}
	rc = pthread_create(&elevator_running_th, NULL, elevator_running_process, NULL);
	if (rc){
		perror("pthread_create");
		exit(-1);
	}

	input_status_t input_read;
	light_status_t light_to_write;
	while(1){

		pthread_mutex_lock(input_event_ptr->mutex);
			pthread_cond_wait(input_event_ptr->cv, input_event_ptr->mutex);

		input_read = get_input_status_unsafe();

 //TODO
		light_to_write = get_light_status();
		for( int floor = 0; floor < N_FLOORS; floor++){
			for( int button_type = 0; button_type < 3; button_type++){
				if(input_read.Button_external[floor][button_type] == 1){/* rising edge */
					light_to_write.floor_button_lights[floor][button_type]
					= (IsRequestAccepted(button_type, floor))? 1 : light_to_write.floor_button_lights[floor][button_type];
				}
			}
		}
		//light_status_t light = get_light_status();
		set_light_status(light_to_write);
		pthread_mutex_unlock(input_event_ptr->mutex);
	}
		
	return 0;
}
