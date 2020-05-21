#include <xinu.h>

future_t** fibfut;
uint future_cons(future_t* fut);
uint future_prod(future_t* fut, char* value);
int ffib(int n);
