#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main() { 
    while(1) { 
        void *m = malloc(1024*1024*10); 
        if(m) memset(m, 1, 1024*1024*10); 
        sleep(1); 
    } 
    return 0; 
}
