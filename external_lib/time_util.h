#ifndef TIME_UTIL_H
#define TIME_UTIL_H

//#include <stdio.h> // debug

//###############################################
// OVERVIEW
//###############################################
/*
struct:
------------------------
timer_data

basic usage:
------------------------
double 	get_time_sec()
void 	wait_sec(double sec)

timer usage:
------------------------
void 	reset_timer(timer_data* timer, double tagret_dt_sec)
void 	update_timer(timer_data* timer)
*/
//###############################################


//###############################################
// DATA TYPE
//###############################################
typedef struct {
double start_time;
double dt_sec_since_start;
double time;
double tagret_dt_sec;
double tagret_time;
double dt_sec;
int at_tagret_time;
} timer_data;


//###############################################
// IMPLEMENTATION
//###############################################

//-----------------------------------------------
// get_time_sec - get current system clock counter time
// wait_sec		- wait for a period in sec
//-----------------------------------------------
#ifdef _WIN32
#include <windows.h>
double get_time_sec(){
static LARGE_INTEGER freq = {0};
if(freq.QuadPart == 0){ QueryPerformanceFrequency(&freq); }
LARGE_INTEGER counter;
QueryPerformanceCounter(&counter);
return (double)counter.QuadPart / (double)freq.QuadPart;
}


void wait_sec(double sec){
Sleep((DWORD)(sec * 1000.0));
}

#elif __linux__ || __linux
#include <time.h>

double get_time_sec(){
struct timespec t;
clock_gettime(CLOCK_MONOTONIC, &t);
return t.tv_sec + t.tv_nsec * 1e-9;
}


void wait_sec(double sec){
struct timespec t;
t.tv_sec = (time_t)sec;
t.tv_nsec = (long)((sec - t.tv_sec) * 1e9);
nanosleep(&t, NULL);
}

#endif


//-----------------------------------------------
// reset_timer - init the timer
//-----------------------------------------------
void reset_timer(timer_data* timer, double tagret_dt_sec){
double time = get_time_sec();
timer->start_time = time;
timer->dt_sec_since_start = 0;
timer->time = time;
timer->tagret_dt_sec = tagret_dt_sec;
timer->tagret_time = time + tagret_dt_sec;
timer->dt_sec = tagret_dt_sec;
timer->at_tagret_time = 0;
return;
}


//-----------------------------------------------
// update_timer - update the timer
//-----------------------------------------------
void update_timer(timer_data* timer){
timer->time = get_time_sec();

if(timer->time >= timer->tagret_time){
double over_time = timer->time - timer->tagret_time;
timer->tagret_time += over_time + timer->tagret_dt_sec;
timer->dt_sec = over_time + timer->tagret_dt_sec;
timer->at_tagret_time = 1;
}
else{
timer->at_tagret_time = 0;
}

timer->dt_sec_since_start = timer->time - timer->start_time;

return;
}

/*
static inline void clock_begin();
static inline void clock_end();
static inline void print_delta_time();
*/


/*
static clock_t begin;
static clock_t end;
static double cpu_time_used;


static inline double get_time(){
return ((double)clock() / CLOCKS_PER_SEC) * 1000;
}

static inline double get_time_sec(){
return (double)clock() / CLOCKS_PER_SEC;
}

static inline void clock_begin(){
begin = clock();
}

static inline void clock_end(){
end = clock();
cpu_time_used = ((double) (end - begin)) / CLOCKS_PER_SEC;
}

static inline void print_delta_time(){
//printf("Time taken: %f seconds \n", cpu_time_used);
}
*/

#endif
