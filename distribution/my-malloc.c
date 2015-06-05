#include <stdio.h>
#include <unistd.h>
#include "my-malloc.h"
#define MOST_RESTRICTING_TYPE double
#define BLOCK_SIZE sizeof(Header)
// Pour s’aligner sur des frontieres multiples
// de la taille du type le plus contraignant
typedef union header {// Header de bloc
	struct{
		unsigned int size; // Taille du bloc
		union header *ptr;// bloc libre suivant
	} info;
	MOST_RESTRICTING_TYPE dummy;// Ne sert qu’a provoquer un alignement
} Header;


/* Une version de my-malloc.c qu'il faudra vite remplacer par quelque chose de
 * plus "raisonnable". Ce code n'est juste là que pour pouvoir lancer
 * les programmes de test.
 */
static int nb_alloc   = 0;		/* Nombre de fois où on alloué     */
static int nb_dealloc = 0;		/* Nombre de fois où on désalloué  */
static int nb_sbrk    = 0;		/* nombre de fois où a appelé sbrk */
static Header sentinelle;

void *mymalloc(size_t size) {
	int sbrkUsage=0;
	static Header *bloc;
	static Header *nextBloc;
	nb_alloc += 1;
	printf("mymalloc(%zu)\n",size);
	if(bloc==NULL){		
		bloc=&sentinelle;
		sentinelle.info.size=0;
		sentinelle.info.ptr=bloc;
	}

	Header *ptr = &sentinelle;
	Header *ptr_prec = &sentinelle;

	while((ptr=ptr->info.ptr)!=&sentinelle){		
		printf("Liste ptr size: %d\n",ptr->info.size);
		ptr_prec=ptr;
		if(ptr->info.size >= (size+BLOCK_SIZE))
			break;		
	}

	if(ptr==&sentinelle){
		if((bloc=sbrk(800))==(void *)-1){
			printf("ERREUR plus de memoire libre\n");
		}
		nb_sbrk++;
		printf("\t-->sbrk(800)\n");
		sbrkUsage=1;
		bloc->info.size=800;
		bloc->info.ptr=&sentinelle;
		ptr_prec->info.ptr=bloc;
	}

	if(sbrkUsage==0){//si on ne fait pas de sbrk
		unsigned int sizePtr = ptr->info.size;
		bloc=ptr;
		if((size/BLOCK_SIZE)*BLOCK_SIZE<size){
			nextBloc=bloc+(size/BLOCK_SIZE)+2;
			bloc->info.size=(size/BLOCK_SIZE)*BLOCK_SIZE + BLOCK_SIZE*2;
		}
		else{
			nextBloc=bloc+(size/BLOCK_SIZE)+1;
			bloc->info.size=(size/BLOCK_SIZE)*BLOCK_SIZE + BLOCK_SIZE;
		}

		nextBloc->info.size=(sizePtr-(size+BLOCK_SIZE));
	}
	else{
		if((size/BLOCK_SIZE)*BLOCK_SIZE<size){
			nextBloc=bloc+(size/BLOCK_SIZE)+2;
			bloc->info.size=(size/BLOCK_SIZE)*BLOCK_SIZE + BLOCK_SIZE*2;
		}
		else{
			nextBloc=bloc+(size/BLOCK_SIZE)+1;
			bloc->info.size=(size/BLOCK_SIZE)*BLOCK_SIZE + BLOCK_SIZE*2;
		}
		nextBloc->info.size=((800)-(size+BLOCK_SIZE));
	}
	if(ptr->info.size!=0)
		nextBloc->info.ptr=ptr->info.ptr;//&sentinelle;
	else
		nextBloc->info.ptr=&sentinelle;
	ptr_prec->info.ptr=nextBloc;
	bloc->info.ptr=nextBloc;
	sentinelle.info.ptr=nextBloc;
	printf("size du bloc retourne %d\n\n",bloc->info.size);
	printf("returned : %d\n\n",bloc);		
	return bloc+1;//on renvois le pointeur sur la zone mémoire donc le +1 est pour la taille du header
}


void myfree(void *ptr) {
  	nb_dealloc += 1;
  	//free(ptr);
	printf("Free de %d\n",ptr);
	Header* pointeur = ptr;
	pointeur = pointeur-1;
	//printf("LE POINTEUR %d : %d\n",pointeur,pointeur->info.size);
	Header *ptr_courant = &sentinelle;
	Header *ptr_prec = &sentinelle;
	/*On parcours les blocs libres*/
	while((ptr_courant=ptr_courant->info.ptr)>pointeur){
		ptr_prec=ptr_courant;//On sauvegarde le bloc libre précedent.
	}
	//TODO check si le bloc avant ou après n'est pas collé et ne peux pas etre fusionne.
	/*On link le nouveau bloc libéré.*/
	pointeur->info.ptr=ptr_courant;
	ptr_prec->info.ptr=pointeur;
	
}

void *mycalloc(size_t nmemb, size_t size) {
  nb_alloc += 1;
  return calloc(nmemb, size);
}

void *myrealloc(void *ptr, size_t size) {
  /* il faudrait probablement changer les valeur de nballoc et
   * nb_dealloc dans une véritable implémentation 
   */
  return realloc(ptr, size);
}

#ifdef MALLOC_DBG
void mymalloc_infos(char *msg) {
  if (msg) fprintf(stderr, "**********\n*** %s\n", msg);

  fprintf(stderr, "# allocs = %3d - # deallocs = %3d - # sbrk = %3d\n",
	  nb_alloc, nb_dealloc, nb_sbrk);
  /* Ca pourrait être pas mal d'afficher ici les blocs dans la liste libre */

  if (msg) fprintf(stderr, "**********\n\n");
}
#endif