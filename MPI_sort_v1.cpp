#include "mpi.h"
#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define SIZE 16384
#define range 100000

int exp2(int num)
{
  	if(num == 1) return 2;
    else if(num == 0) return 1;
    else{
        if(num%2 == 0){
            int temp = exp2(num/2);
            return temp*temp;
        }
        else{
            int temp = exp2(num/2);
            return temp*temp*2;
        }
    }
}


//log2num
int log2(int num)
{
  	int i=1;
  	int j=2;
  	while(j<num){
  		j=j*2;
  		i++;
  	}
  	if(j>num)
  		i--;
  	return i;
}

bool Is_sort(int *array){
	for(int i = 0;i < SIZE-1;i++){
		if(array[i+1] < array[i]){
			return false;
		}
	}
	return true;
}

void swap(int *a,int *b){
	int t = *a;
	*a = *b;
	*b = t;
}

int get_privot(int* array,int begin,int end){
	int privot = array[begin];
    int i = begin+1;
    int j = end;
    while(i <= j){
    while((i <= end) && (array[i] <= privot))
            i++;
    while((j >= begin) && (array[j] > privot))
            j--;
    if(i < j)
            swap(&array[i],&array[j]);
    }
    swap(&array[begin],&array[j]);
	return j;
}

void quicksort(int* array,int begin,int end){
	if(begin >= end){
		return;
	}
	else{
		int privot = get_privot(array,begin,end);
	    quicksort(array,begin,privot-1);
	    quicksort(array,privot+1,end);
	}
}

int* generate_random(int num){
	srand(time(NULL) + rand());
	int *a = (int*)malloc(num*sizeof(int));
	int i = 0;
	for(i = 0;i < num;i++){
		a[i] = rand()%range;
	}
	return a;
}

void sort_recursive(int* array, int begin,int end, int currProcRank, int maxRank, int sqrt_num_process) {
	MPI_Status status;
	int shareProc = currProcRank + Exp2(sqrt_num_process-1);
	if (shareProc > maxRank||sqrt_num_process==0) {
		quicksort(array, begin,end);
		return;
	}

    sqrt_num_process--;
    MPI_Send(&sqrt_num_process, 1, MPI_INT, shareProc, 100, MPI_COMM_WORLD);

	int pivotIndex = get_privot(array,begin,end);
    int len1 = pivotIndex - begin;
    int len2 = end = pivotIndex;

	if (len1 < len2) {
		MPI_Send(array, len1, MPI_INT, shareProc, MPI_ANY_TAG, MPI_COMM_WORLD);
		sort_recursive(array, pivotIndex + 1,end, currProcRank, maxRank, sqrt_num_process);
		MPI_Recv(array, len1, MPI_INT, shareProc, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}
	else {
		MPI_Send((array + len1 + 1), len2, MPI_INT, shareProc, MPI_ANY_TAG, MPI_COMM_WORLD);
		sort_recursive(array, begin,pivotIndex-1, currProcRank, maxRank, sqrt_num_process);
		MPI_Recv((array + len1 + 1), len2, MPI_INT, shareProc, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}
}

int main(int argc, char **argv) {	
	
	int size, rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
    int *array;
	double start_time,end_time;
	double start_time1,end_time1;

    if (rank == 0) {
        int *temp  = generate_random(SIZE);
		start_time1 = MPI_Wtime();
		quicksort(temp,0,SIZE-1);
		end_time1 = MPI_Wtime();
		printf("serial time for size of %d is: %fs\n",SIZE,end_time1-start_time1);
        free(temp);
        array = generate_random(SIZE);
        printf("begin MPI_sort\n");
        start_time = MPI_Wtime();
        sort_recursive(array,0,SIZE-1,0,size,log2(size));
        end_time = MPI_Wtime();
        printf("end MPI_sort\n");
        printf("parallel time using %d process for size of %d is: %fs\n",size,SIZE,end_time1-start_time1);
        if(Is_sort(array)){
            std::cout<<"sucessfully sort\n";
        }
    }
    else {
        MPI_Status status;
        int sqrt_num_process;
        MPI_Recv(&sqrt_num_process,1,MPI_INT,MPI_ANY_SOURCE,100,MPI_COMM_WORLD,&status);

        int subarray_size;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &subarray_size);
        
        int source_process = status.MPI_SOURCE;
        int *subarray = (int*)malloc(subarray_size * sizeof(int));

        MPI_Recv(subarray, subarray_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        sort_recursive(subarray,0,subarray_size-1, rank, size, sqrt_num_process);
        MPI_Send(subarray, subarray_size, MPI_INT, source_process, 0, MPI_COMM_WORLD);
    }
	MPI_Finalize();
    return 0;
}