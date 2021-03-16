#include "buddy.h"

int main(){
    
    void* space=calloc(1000,BLOCK_SIZE);
    buddy_init(space,1000);
    print_buddy();
    /*for(int i=1;i<10;i*=2){
        buddy_alloc(i*BLOCK_SIZE);
        printf("%d ALLOCATION! \n",i);
        print_buddy();
    }
    */

    //print_buddy();
    return 0;
}