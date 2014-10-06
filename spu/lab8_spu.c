#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libmisc.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#define waitag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();
#define BLOC 8


void* _alloc(int size){
	void *res;

	res = malloc_align(size,4);
	if (!res){
		fprintf(stderr, "%s: Failed to allocated %d bytes\n", __func__,
				size);
		exit(0);
	}

	return res;
}

typedef struct {
    short int *point;
    short int *output;
    struct block *blocuri;
    int dim;
    int width;
}__attribute__ ((aligned(16))) param_t;


struct block{
	//data for a block from the compressed image
	unsigned char a, b;
	unsigned char bitplane[BLOC * BLOC];
	//one byte for each bit in the bitplane
	//quite memory inefficient, but let's keep it simple
}__attribute__ ((aligned(16)));

struct c_img{
	//compressed image
	int width, height;
	struct block* blocks;
}__attribute__ ((aligned(16)));

struct bits{
	unsigned bit0 : 1;
	unsigned bit1 : 1;
	unsigned bit2 : 1;
	unsigned bit3 : 1;
	unsigned bit4 : 1;
	unsigned bit5 : 1;
	unsigned bit6 : 1;
	unsigned bit7 : 1;
} __attribute__ ((aligned(16)));


mfc_list_element_t list[16] __attribute__((aligned(16)));


//functia pentru transfer folosind liste dma care apeleaza getl
void large_transfer( void *LS, unsigned long long EA, unsigned int nbytes, uint32_t tag_id) {
	unsigned int i = 0;
	unsigned int listsize;
	unsigned int sz;
	unsigned int ealow = mfc_ea2l(EA);
	
	while( nbytes > 0 ) {
		sz = (nbytes < 16384) ? nbytes : 16384;
		list[i].size = sz;
		list[i].eal = ealow;
		nbytes -= sz;
		ealow += sz;
		i++;
	}

	listsize = i * sizeof(mfc_list_element_t);
	mfc_getl(LS, EA, list, listsize, tag_id, 0, 0);
	mfc_write_tag_mask(1 << tag_id);
	mfc_read_tag_status_any();
}

//functie pentru transfer folosind liste dma care apeleaza putl
void large_transfer2( void *LS, unsigned long long EA, unsigned int nbytes, uint32_t tag_id) {
	unsigned int i = 0;
	unsigned int listsize;
	unsigned int sz;
	unsigned int ealow = mfc_ea2l(EA);
	
	while( nbytes > 0 ) {
		sz = (nbytes < 16384) ? nbytes : 16384;
		list[i].size = sz;
		list[i].eal = ealow;
		nbytes -= sz;
		ealow += sz;
		i++;
	}
	listsize = i * sizeof(mfc_list_element_t);
	mfc_putl(LS, EA, list, listsize, tag_id, 0, 0);
	mfc_write_tag_mask(1 << tag_id);
	mfc_read_tag_status_any();
}

//functie ce calculeaza suma unui vector
int suma_elemente(vector signed short vec){
	return vec[0]+vec[1]+vec[2]+vec[3]+vec[4]+vec[5]+vec[6]+vec[7];
}
//functie pentru afisare a unui vector folosita in pentru debugg
void afisare(vector signed short vec){
	int i;
	for(i=0;i<8;i++)
		printf("%d ",vec[i]);
	printf("\n");
}

int main(unsigned long long speid, unsigned long long argp, unsigned long long envp)
{

	param_t p __attribute__ ((aligned(16)));
	uint32_t tag_id = mfc_tag_reserve();
	int i,j,k,l,bt_index;
	float mean, dev;
	int indice, block_row_start, block_col_start, it, it2;
	float as=0,b=0,f1,f2,q;

	short int *a __attribute__ ((aligned(16)));
	int dimensiune_lista, nr_bw, index_bloc;

	short int vec[64] __attribute__ ((aligned(16)));
	vector signed short v[8] __attribute__ ((aligned(16)));
	vector signed short suma __attribute__ ((aligned(16)));
	vector signed short suma_deviatie __attribute__ ((aligned(16)));
	vector signed short vmedie __attribute__ ((aligned(16)));
	vector signed short diferenta[8] __attribute__ ((aligned(16)));
	struct block *blocks __attribute__ ((aligned(16)));
	short int *out __attribute__ ((aligned(16)));

	vmedie = (vector signed short) spu_splats((signed short) 0);
	suma= (vector signed short) spu_splats((signed short) 0);
	suma_deviatie = (vector signed short) spu_splats((signed short) 0);

	if (tag_id==MFC_TAG_INVALID){
		printf("SPU: ERROR can't allocate tag ID\n"); return -1;
	}
	/*prim transfer cu structura ce contine toate informatiile necesare:
		-pointer catre partea din imagine ce trebuie prelucrata de acest spu
		-pointer catre partea din imaginea de output pgm prelucrata de acest spu
		-dimensiunea width a imaginii
		-pointer catre partea din imaginea compressed prelucrata de acest spu, catre structura block
		 de inceput
		-dim reprezinta numarul de linii-bloc prelucrate de acest spu. O linie-bloc contine 8 linii
		normale. deci size(linie bloc) = 8*width
	
	*/
	mfc_get(&p, argp, sizeof(param_t), tag_id, 0, 0);
    waitag(tag_id);
    tag_id = mfc_tag_reserve();

    //alocare dimensiuni pentru buffere folosite
	dimensiune_lista=BLOC*p.width*sizeof(short int);
	a=(short int*)_alloc(dimensiune_lista);
	nr_bw = p.width/BLOC;
	blocks = (struct block*)_alloc(nr_bw*sizeof(struct block));
	out=(short int*) _alloc(BLOC*p.width*sizeof(short int));

	//se transfera cate o linie bloc(8 linii din matrice) in bufferul a
	index_bloc=0;
	for(i=0; i<p.dim; i++){
		large_transfer(a,(unsigned long long )p.point + i*BLOC*p.width*sizeof(short int), dimensiune_lista, tag_id);
		index_bloc=0;

		//se calculeaza media si deviatia pentru fiecare bloc
		mean = 0;
		for(k=0;k<p.width;k+=BLOC){
			indice=0;
			for(j=0;j<BLOC;j++){
				for(l=k;l<k+BLOC;l++){
					vec[indice] = a[j*p.width+l];
					indice++;
				}

			}
			
			int adunare=0;
			//se copiaza in vector array-ul ce contine pixelii din bloc
			memcpy(v, vec, BLOC*BLOC*sizeof(short int));
			suma = (vector signed short) spu_splats((signed short) 0);
			for(it=0 ;it<8; it++){
				suma = spu_add(suma,v[it]);
			}
			//se face suma lor
			adunare=suma_elemente(suma);
			mean =(float) adunare / (BLOC * BLOC);
		//se calculeaza deviatia
		dev = 0;
		suma = (vector signed short) spu_splats((signed short) 0);
		vmedie = (vector signed short) spu_splats((signed short) mean);
		
		for(it=0 ;it<8; it++){
			diferenta[it] = spu_sub(v[it],vmedie);
		}
		//se ridica la patrat fiecare element din vectorul diferenta
		signed int vv[64];
		for(it = 0; it <8 ;it++)
			for(it2 = 0; it2 <8 ;it2++)
				vv[it*8+it2] = diferenta[it][it2];
		for(it = 0; it <64 ; it++){
			vv[it] = vv[it] * vv[it];
		}
		adunare = 0;
		for(it = 0; it <64 ; it++)
			adunare+=vv[it];
		dev = (float)adunare/(BLOC*BLOC);
		dev = sqrt(dev);
		/*se calculeaza bitplane, a si b
		a la mine se numeste as pentru ca am folosit numele de a la bufferul
		in care se primeste linia bloc
		*/
		bt_index = 0;
		q = 0;
		unsigned char tmp;
		for(it=0; it<64; it++){
			tmp = (vec[it] > mean ? 1 : 0);
			blocks[index_bloc].bitplane[bt_index] = tmp;
			q += tmp;
			bt_index++;		
		}
		
		if (q == 0) {
            as = mean;
            b = mean;
        } 
        else 
        {
	    	f1 = sqrt(q / (64 - q));
        	f2 = sqrt((64 - q) / q);
        	as = (mean - dev * f1);
    		b = (mean + dev * f2);
        }

        if (as < 0) 
        	as = 0;
        if (b > 255) 
        	b = 255;
      	blocks[index_bloc].a=(unsigned char)as;
       	blocks[index_bloc].b=(unsigned char)b;

        index_bloc++;
		}

		//se trimite in memoria din ppu
		large_transfer2(blocks, (unsigned long long) p.blocuri+i*nr_bw*sizeof(struct block),nr_bw*sizeof(struct block),tag_id);
		
		//decompresie
		block_row_start = 0;
		block_col_start = 0;
		for(indice=0;indice<nr_bw;indice++){
			it=block_row_start * p.width + block_col_start;
			for (it2=0; it2<BLOC * BLOC; it2++){
				if(blocks[indice].bitplane[it2] == 1){
					out[it] = blocks[indice].b;
					it++;
				}
				else {
					out[it]= blocks[indice].a;
					it++;
				}
				if((it2+1) % BLOC ==0){
					it -= BLOC;
					it += p.width;
				}
			}
			block_col_start += BLOC;

		}
		//trimitere ppu
		large_transfer2(out, (unsigned long long) p.output+i*BLOC*p.width*sizeof(short int),BLOC*p.width*sizeof(short int),tag_id);
	}
	free_align(a);
	free_align(out);
	free_align(blocks);
	mfc_tag_release(tag_id);
	return 0;
}
