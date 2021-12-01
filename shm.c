#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

//you write this LAB 4 MODIFIED

    char exist = 0;     //checks for the existance of the id
    int i = 0;          //index page

    acquire(&(shm_table.lock)); //get the lock for the shm table

    //CASE 1 WHERE ID EXISTS ALREADY

    //start by looping through the table
    for(i= 0; i < 64; ++i){         //If the id matches the id of the page table, check that it was case 1 and then exit
        if(shm_table.shm_pages[i].id == id){
            exist = 1;
            break;
        }
    }

    //CASE 2 WHERE SHARED MEM SEG DNE
    if(!exist){
        for(i = 0; i < 64; ++i){                        //loop through the table to find the page again
            if(shm_table.shm_pages[i].id == 0){
                char *page = kalloc();                  //allocate a new page
                memset(page, 0, PGSIZE);                //MAP TO 0: initialize + write
                shm_table.shm_pages[i].frame = page;    //New page gets assigned the fram
                shm_table.shm_pages[i].id = id;         //page gets ID and exit
                break;
            }
        }
    }

    //NEXT: we need to map the id to the memory in the page table
    //Here we reference mmu.h
    mappages(
            myproc()->pgdir,                     //getting the pointer of pagedir
            (void *)PGROUNDUP(myproc()->sz),     //gets the virtual adrr of page
            PGSIZE,                              //size of mapping
            V2P(shm_table.shm_pages[i].frame),   //gets the physical address to point to
            (PTE_W | PTE_U)                      //sets the permissions writtable and accessable to user
            );

    //NEXT: we need to increment the reference count and have a pointer to the virtual address
    shm_table.shm_pages[i].refcnt++;    
    *pointer = (char *)PGROUNDUP(myproc()->sz);        

    //Current process needs to be updated and we need to then unlock shm table

    myproc()->sz += PGSIZE;
    release(&(shm_table.lock));

return 0;
}

int shm_close(int id) {

//you write this too! LAB 4 MODIFIED

int i;

//start by looping through the table and check if the ID is found
//While in the for loop, check if the memory is shared and decrement the reference count.
for(i = 0; i < 64; ++i){
    if(shm_table.shm_pages[i].id == id){
        if(shm_table.shm_pages[i].refcnt > 1){
            --shm_table.shm_pages[i].refcnt;
        }

        //Set the values to 0 to release the pages

        else{
            shm_table.shm_pages[i].id = 0;
            shm_table.shm_pages[i].frame = 0;
            shm_table.shm_pages[i].refcnt = 0;
        }
        break;
    }
}

return 0;
}