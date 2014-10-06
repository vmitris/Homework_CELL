#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libspe2.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libmisc.h>
#include "btc.h"


#define BLOC 8
extern spe_program_handle_t lab8_spu;

/*strucctura folosita pentru comunicarea cu spu
**point reprezinta pointer la partea din imaginea de intrare
**output reprezinta pointer la partea din imaginea de iesire dupa decompresie
** reprezinta pointer la structura block de unde incepe fiecare spu procesarea
*/
struct  param_t{
    short int *point;
    short int *output;
    struct block *blocuri;
    int dim;
    int width;
}__attribute__ ((aligned(16)));


//funtia care creeaza contextul
void *functie(void *thread_arg) {

    spe_context_ptr_t ctx;
    struct param_t *arg = (struct param_t *) thread_arg;

    if ((ctx = spe_context_create (0, NULL)) == NULL) {
            perror ("Failed creating context");
            exit (1);
        }

    if (spe_program_load (ctx, &lab8_spu)) {
            perror ("Failed loading program");
            exit (1);
        }

    unsigned int entry = SPE_DEFAULT_ENTRY;
    if (spe_context_run(ctx, &entry, 0, (void *)arg, NULL, NULL) < 0) {
        perror ("Failed running context");
        exit (1);
    }

    if (spe_context_destroy (ctx) != 0) {
            perror("Failed destroying context");
            exit (1);
        }

    pthread_exit(NULL);
}


int main(int argc, char **argv)
{

    //folosite pentru calculul timpului
    struct timeval t1, t2, t3, t4;
    double total = 0, total_mic = 0;
    //argumentele din linia de comanda
    int num_spus=atoi(argv[2]);
    char *in=argv[3];
    char *out_btg=argv[4];
    char *out_pgm=argv[5];
    //imaginea citita
    struct img image;
    struct img out_image;
    int i, cat, nr_blocuri;
    pthread_t threads[num_spus] __attribute__ ((aligned(16)));
    struct param_t structura[num_spus] __attribute__ ((aligned(16)));
    struct c_img compress __attribute__ ((aligned(16)));
    gettimeofday(&t1, NULL);
    read_pgm(in, &image);

    printf("numar linii = %d\n",image.height);
    printf("numar coloane = %d\n",image.width);
    
    nr_blocuri = image.height*image.width/(BLOC*BLOC);
    printf("numar BLOCURI = %d\n",nr_blocuri);
    cat = image.height / ( BLOC * num_spus);

    compress.blocks = (struct block*) _alloc(sizeof(short int) * nr_blocuri * sizeof(struct block));
    //short int *out=(short int*)_alloc(image.width*image.height*2);
    out_image.pixels = (short int*)_alloc(image.width*image.height*sizeof(short int));
    gettimeofday(&t3, NULL);
    /*se calculeaza numarul de linii-bloc(variabila cat) pe care le proceseaza fiecare spu
    *o linie bloc contine 8 linii normale din matrice
    *ultimul spu va avea mai multe linii bloc, in cazul in care height un se imparte exact la 64
    *se calculeaza pointerii pentru fiecare spu si se trimite structura 
    */
    compress.height = image.height;
    compress.width = image.width;
    out_image.height = image.height;
    out_image.width = image.width;
    
    for(i=0;i<num_spus; i++){
        structura[i].point= image.pixels+i*cat*image.width*BLOC;
        structura[i].width = image.width;
        structura[i].output = out_image.pixels+i*cat*image.width*BLOC;
        structura[i].blocuri = compress.blocks + i*cat*image.width/8;
        if(i != num_spus-1){
            structura[i].dim = cat;
        }
        else{
            structura[i].dim = (image.pixels+image.width*image.height-structura[i].point)/(BLOC*image.width);

        }
        if (pthread_create (&threads[i], NULL, &functie, &structura[i]))  {
            perror ("Failed creating thread");
            exit (1);
        }
    }

    for (i = 0; i < num_spus; i++) {
        if (pthread_join (threads[i], NULL)) {
            perror("Failed pthread_join");
            exit (1);
        }
    }
    gettimeofday(&t4, NULL);
    write_btc(out_btg, &compress);
    write_pgm(out_pgm, &out_image);

    gettimeofday(&t2, NULL);
    //se calculeaza timpii
    total += GET_TIME_DELTA(t1, t2);
    total_mic += GET_TIME_DELTA(t3, t4);
    //se afiseaza timpii
    printf("Total time :%lf\n", total);
    printf("E/D time :%lf\n", total_mic);
    return 0;
}
