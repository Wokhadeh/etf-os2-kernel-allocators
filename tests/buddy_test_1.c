#include "buddy.h"

int main()
{

    void *space = calloc(1000, BLOCK_SIZE);
    void* addres[1000];
    int cnt=0;
    //printf("Pointer %p  and size %llu and sizeof longint %llu \n", space, sizeof(space), sizeof(long long int));
    buddy_init(space, 1000);
    print_buddy();
  
  
  
  
    for(int i=1;i<10;i*=2){
        addres[cnt++]=buddy_alloc(i*BLOCK_SIZE);
        
        printf("%d ALLOCATION! \n",i);
        print_buddy();
    }
    printf("*****************DEALOKACIJA*****************\n");
    for(int i=0;i<cnt;i++){
        buddy_free(addres[i]);
        print_buddy();
    }

    
/*    for (int i = 0; i < 9; i++)
        printf("Buddy of space %d : %p  is %p \n",i, ((buddy_info *)kernel_space)->blocks[i].next,
               find_buddy(((buddy_info *)kernel_space)->blocks[i].next));
*/
    //print_buddy();
    return 0;
}