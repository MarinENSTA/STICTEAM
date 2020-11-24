#include <stdio.h>
#include <stdlib.h>
// Déclarer une zone d'espace mémoire de taille fixe : un kilo-octet = 1024 octets.
// On utilise char plutôt que char* car le premier alloue un tableau alors que le deuxième alloue un espace pour un pointeur,
// qui ne nécessite que 4 octets en mémoire. On va donc utiliser char afin de définir un espace de grande taille qui correspond au tableau.
// Quelques subtilités sur les char et char* :
//
// char heap[16*1024]; tableau de taille 16ko
// char *ptr;
// char *ptr2 = ptr + 4;
// 
// int main(){
// char x = heap[4];
// ptr = &heap;
// char y = ptr[4];
// char z = ptr2[0];
// }
// 
// x, y, et z correspondent au même élément. Pour accéder à x, on utilise deux déplacements dans la mémoire, pour y, un seul, pour z, un seul.
// 
char heap[16*1024];

#if 0

int allocated = 0;

void *memalloc(int size){
    int new_al = allocated + size;
    if(new_al > 16*1024) {
        printf("Erreur : plus de place.\n");
        exit(1);
    }
    char *ret = &heap[allocated];
    allocated = new_al;
    return ret;
}

void memfree(char *data){}

#endif

struct point {
    int x;
    int y;
};

struct zone {
    int size;
    struct zone* next;
};

struct zone* first_zone;

void meminit(){
    struct zone* z = (struct zone*) &heap[0];
    z->size = 16*1024 - sizeof(struct zone);
    z->next = NULL;
    first_zone = z;
}

void *memalloc(int size){
    struct zone **z = &first_zone;
    struct zone *zz = *z;
    while(zz != NULL && zz->size < size){zz = zz->next;}
    if (*z == NULL){return (void *) NULL;} // Pas de zone de taille suffisante "size" à allouer
    if (zz->size >= size + sizeof(struct zone)){
        char *ret = (char *) *z + zz->size - size;
        zz->size = zz->size - size;
        return ret;
    }
    // else {printf("TODO\n");exit(1);}
    else {
        void *ret = *z;
        *z = zz->next;
        return ret;
    }
}

void memfree(void *ptr){
    int *size_ptr = (int *)((char *)ptr - sizeof(int));
    int size = *size_ptr;
    struct zone *z = (struct zone *) ptr;
    z->size = size;
    z->next = first_zone;
    first_zone = z;
}

int main(){
    char heap[16*1024]; // tableau de taille 16ko
    char *ptr;
    char *ptr2 = ptr + 4;
    #if 0
       if (p == NULL) {printf("Plus de place");};
            struct point *p = memalloc(sizeof(struct point));
            char x = heap[4];
            ptr = heap; // ptr = &heap[0]; convient aussi
            char y = ptr[4];
            char z = ptr2[0];
        }
    #endif
    return 0;
}
