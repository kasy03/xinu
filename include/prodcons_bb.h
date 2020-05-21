#include <xinu.h>

extern sid32 psems;
extern sid32 csems;
extern sid32 mutex; 
#define buff_size 5
extern int readIndex;
extern int writeIndex;
extern arr_q[buff_size];


void consumer_bb(int count);
void producer_bb(int count);
