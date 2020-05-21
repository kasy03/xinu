#include <xinu.h>
#include <future.h>


future_t* future_alloc(future_mode_t mode, uint size, uint nelems){
	intmask  mask;  
	struct future_t* f = (struct future*)getmem(sizeof(struct future_t));
	
	mask = disable();
	if(f==NULL){	
		restore(mask);
		return SYSERR;
	}
	
	f->data = (char*)getmem(sizeof(char)*size*nelems);
	f->size = size;	
	f->mode = mode;
	f->state = FUTURE_EMPTY;
	f->set_queue = NULL;
	f->get_queue = NULL;
	f->pid = 0;
	f->max_elems = nelems;
	f->count =0;
	f->head =0; 
	f->tail = 0;
	restore(mask);
	return f;

}
syscall future_free(future_t* future){
		intmask  mask; 

		mask = disable();
		if(future==NULL){
			
			restore(mask);
			return SYSERR;
		}else{
			
			if(future->set_queue!=NULL){
				struct queue* element;
				while(future->set_queue){
					element = future->set_queue->next;
					freemem((char*)future->set_queue,sizeof(struct queue));
					future->set_queue = element;
				}
			}
			 
			 if(future->get_queue!=NULL){
				struct queue* element;
				while(future->get_queue){
					element = future->get_queue->next;
					freemem((char*)future->get_queue,sizeof(struct queue));
					future->get_queue = element;
				}
			 }
			 freemem((char*)future, sizeof(struct future_t));
			 restore(mask);
			 return OK;
		}
}
syscall future_get(future_t* f, char* in){
	intmask  mask; 
	
	mask = disable();
	if(f==NULL){
		restore(mask);
		return SYSERR;
	}
	if(f->mode == FUTURE_EXCLUSIVE){
			
			if(f->pid!=0){
				restore(mask);
				return SYSERR;
			}
			
			f->pid = getpid();
			if(f->state!= FUTURE_READY)
			
				suspend(f->pid);
				
			memcpy(in, (f->data), f->size);
	}else if(f->mode == FUTURE_SHARED){
		
			if(f->state != FUTURE_READY){
				pid32 p = getpid();
				if(f->get_queue==NULL){
					f->get_queue = (struct queue*)getmem(sizeof(struct queue));
					f->get_queue->next = NULL;
					f->get_queue->pid = p;
				
				}else{
					struct queue* tmp = f->get_queue;
					while (tmp->next){
						tmp = tmp->next;
					}
					tmp->next = (struct queue*)getmem(sizeof(struct queue));
					tmp->next->pid = p;
					tmp->next->next = NULL;
				}
				suspend(p);
			}
			memcpy(in, (f->data), f->size);
	}else if(f->mode == FUTURE_QUEUE){
		if(f->count==0){
				pid32 p = getpid();
				if(f->get_queue==NULL){
					f->get_queue = (struct queue*)getmem(sizeof(struct queue));
					f->get_queue->next = NULL;
					f->get_queue->pid = p;
				
				}else{
					struct queue* tmp = f->get_queue;
					while (tmp->next){
						tmp = tmp->next;
					}
					tmp->next = (struct queue*)getmem(sizeof(struct queue));
					tmp->next->pid = p;
					tmp->next->next = NULL;
				}
				f->state = FUTURE_WAITING;
				suspend(p);
				
		}	
		//ready 
		if(f->count>0){	
			char* headelemptr = f->data + (f->head * f->size);
			memcpy(in, headelemptr, f->size);
			f->head = (f->head +1) % f->max_elems;
			f->count = f->count-1;
			if(f->set_queue!=NULL){
				struct queue* tmp = f->set_queue;
				f->set_queue = f->set_queue->next;
				if (tmp->pid!=0)
					resume(tmp->pid); 
			}
		}
	}else{
			restore(mask);
			return SYSERR;
	}	
	restore(mask);
	return OK;
	
}	
syscall future_set(future_t* f, char* in){
	intmask  mask; 
	
	mask = disable();
	if(f==NULL){
		restore(mask);
		return SYSERR;
	}
	if(f->mode== FUTURE_EXCLUSIVE){
			if (FUTURE_EMPTY != f->state)
			{
				restore(mask);
				return SYSERR;
			}
			f->state = FUTURE_WAITING;	
			memcpy((f->data),in , f->size);
			f->state = FUTURE_READY;	
			if (f->pid!=0)
				resume(f->pid);
	}else if(f->mode == FUTURE_SHARED){
		
			if (FUTURE_EMPTY != f->state){
				
				restore(mask);
				return SYSERR;
			}
			f->state = FUTURE_WAITING;	
			memcpy((f->data),in, f->size);
			f->state = FUTURE_READY;
			struct queue* tmp = f->get_queue;
			while (tmp){
				f->get_queue = f->get_queue->next;
				
				resume(tmp->pid);
				freemem((char*)tmp, sizeof(struct queue));
				tmp = f->get_queue;
			}
	}else if(f->mode == FUTURE_QUEUE){
			if(f->count==f->max_elems){
				pid32 p = getpid();
				if(f->set_queue==NULL){
					f->set_queue = (struct queue*)getmem(sizeof(struct queue));
					f->set_queue->next = NULL;
					f->set_queue->pid = p;
				
				}else{
					struct queue* tmp = f->set_queue;
					while (tmp->next){
						tmp = tmp->next;
					}
					tmp->next= (struct queue*)getmem(sizeof(struct queue));
					tmp->next->pid = p;
					tmp->next->next = NULL;
				}
				f->state = FUTURE_WAITING;
				suspend(p);
				
				
			}
			//no space 	
			if(f->count<f->max_elems){
				char* tailelemptr = f->data + (f->tail * f->size);
				memcpy(tailelemptr, in, f->size);
				f->tail = (f->tail +1)% f->max_elems;
				f->count = f->count +1;
				f->state = FUTURE_READY; 
				if(f->get_queue!=NULL){
					struct queue* tmp = f->get_queue;
					f->get_queue = f->get_queue->next;
					if (tmp->pid!=0)
						resume(tmp->pid); 
				}
			}
	}
	else{
			restore(mask);
			return SYSERR;
	}
	restore(mask);
	return OK;
}
