/*
!    Copyright (c) 2009-2012 by High Performance Computing Virtual Laboratory
!
!    This is a square root summation example,
!    performed by subgroups of processes in MPI
!    parallelism, by employing special case 3
!    of the DMSM library.
!
!    In this test, there are three input data
!    files. One is
!    MPI.PROCESSES.IN.JOB.COMM.dat
!    which containes only one integer as it names.
!
!    Another input data file is in.dat, which
!    containes two lines of integers as follows.
!
!    The first line is the JOB_DISTRIBUTION_PLAN
!    which can only take 11, 12, 13, 21, 22, 23,
!    31, 32, or 33.
!
!    The second line is the total number of jobs,
!    and number of jobs in each job group.
!
!    The last file is integers.dat, which is
!    the upper bound for square root summation.
!
!    The comment lines of the file dmsm.c is
!    a little more detailed description of the
!    DMSM model and its examples.
*/


#include "dmsm.h"
#include <mpi.h>
#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>


int  My_MPI_Rank, Total_MPI_Processes;
int  Job_Distribution_Plan;
int  Total_Num_Of_Jobs,Num_Of_Jobs_Per_Group;
long *Initial_Data_For_Jobs;
long *Initial_Data_Of_A_Job_Group;

int  *Final_Total_Jobs_In_Master;
double *Final_Total_Results_In_Master;
int  Total_Of_Final_Results_In_Master;

int  Number_of_Results_in_Process;
int *Jobs_in_Process;
double *Results_in_Process;

int Total_Processes_For_a_Job;
int  All_Rank, All_Processes;
MPI_Comm Job_Communicator;
int      Job_Rank, Job_Processes;
MPI_Comm Dist_Communicator;
int      Dist_Rank, Dist_Processes;


void Initial_MPI(int *rank, int *totps);
void Data_Initialize();
void Do_The_Job(int my_job);
void Prepare_Job_Group_Data(int from, int to, int my_rank, int destination);
void Collect_Some_Results(int my_rank, int from);
void Output_Results();
void Data_Finalize();


int main(argc,argv)
int argc;
char *argv[];
{
  MPI_Init(&argc,&argv);
  Initial_MPI(&My_MPI_Rank, &Total_MPI_Processes);
  Data_Initialize();
  DMSM_Gen_Comm_MPI_All(Total_Processes_For_a_Job,
                           MPI_COMM_WORLD,  &All_Rank,  &All_Processes,
                        &Job_Communicator,  &Job_Rank,  &Job_Processes,
                       &Dist_Communicator, &Dist_Rank, &Dist_Processes);
  if(All_Rank == 0) printf(" %d %d \n", All_Rank, Job_Processes);
  if(All_Rank == 1) printf(" %d %d \n", All_Rank, Job_Processes);
  if(All_Rank == All_Processes-1) printf(" %d %d \n", All_Rank, Job_Processes);
  DMSM_MPI_All(Total_Num_Of_Jobs,
               Num_Of_Jobs_Per_Group,
               Do_The_Job,
               Prepare_Job_Group_Data,
               Collect_Some_Results,
               1);
  Output_Results();
  Data_Finalize();
  MPI_Finalize();
  return(0);
}




void Initial_MPI(int *rank, int *totps)
{
  MPI_Comm_rank(MPI_COMM_WORLD,rank);
  MPI_Comm_size(MPI_COMM_WORLD,totps);
}




void Data_Initialize()
{ int i,j;
  FILE * fin;

  if(My_MPI_Rank==DMSM_Get_Master())
    {
        fin=fopen("MPI.PROCESSES.IN.JOB.COMM.dat","r");
        fscanf(fin,"%d \n",&Total_Processes_For_a_Job);
        fclose(fin);

        fin=fopen("in.dat","r");
        fscanf(fin,"%d \n",&Job_Distribution_Plan);
        fscanf(fin,"%d %d\n",&Total_Num_Of_Jobs,&Num_Of_Jobs_Per_Group);
        fclose(fin);
    }

  MPI_Bcast(&Total_Num_Of_Jobs,     1, MPI_INT, DMSM_Get_Master(), MPI_COMM_WORLD);
  MPI_Bcast(&Num_Of_Jobs_Per_Group, 1, MPI_INT, DMSM_Get_Master(), MPI_COMM_WORLD);

  if(My_MPI_Rank==DMSM_Get_Master())
     {if((Initial_Data_For_Jobs=(long *) malloc(Total_Num_Of_Jobs*sizeof(long)))==NULL)
         {printf( "Error on memory allocation in Data_Initialize().\n") ;
          MPI_Finalize(); exit(0);}
      fin=fopen("integers.dat","r");
      for(i=0;i<Total_Num_Of_Jobs;i++)
         fscanf(fin," %d %ld \n", &j, &Initial_Data_For_Jobs[i]);
      fclose(fin);

      if((Final_Total_Jobs_In_Master=(int *)  malloc(Total_Num_Of_Jobs*sizeof(int)))==NULL)
         {printf( "Error on memory allocation in Data_Initialize().\n") ;
          MPI_Finalize(); exit(0);}

      if((Final_Total_Results_In_Master=(double *)  malloc(Total_Num_Of_Jobs*sizeof(double)))==NULL)
         {printf( "Error on memory allocation in Data_Initialize().\n") ;
          MPI_Finalize(); exit(0);}
      }

  else
     {if((Final_Total_Jobs_In_Master=(int *)  malloc(1*sizeof(int)))==NULL)
        {printf( "Error on memory allocation in Data_Initialize().\n") ;
         MPI_Finalize(); exit(0);}
      if((Final_Total_Results_In_Master=(double *)  malloc(1*sizeof(double)))==NULL)
        {printf( "Error on memory allocation in Data_Initialize().\n") ;
         MPI_Finalize(); exit(0);}
      }

  if((Initial_Data_Of_A_Job_Group=(long *) malloc(Num_Of_Jobs_Per_Group*sizeof(long)))==NULL)
    {printf( "Error on memory allocation in Data_Initialize().\n") ;
     MPI_Finalize(); exit(0);}

  Total_Of_Final_Results_In_Master=0;

  Number_of_Results_in_Process=0;

  if((Jobs_in_Process=(int *) malloc(Num_Of_Jobs_Per_Group*sizeof(int)))==NULL)
              {printf( "Error on memory allocation in Data_Initialize().\n") ;
               MPI_Finalize(); exit(0);}
  if((Results_in_Process=(double *) malloc(Num_Of_Jobs_Per_Group*sizeof(double)))==NULL)
              {printf( "Error on memory allocation in Data_Initialize().\n") ;
               MPI_Finalize(); exit(0);}

}




void Data_Finalize()
{
  if(My_MPI_Rank==DMSM_Get_Master())
  free(Initial_Data_For_Jobs);
  free(Initial_Data_Of_A_Job_Group);
  free(Final_Total_Results_In_Master);
  free(Final_Total_Jobs_In_Master);
  free(Jobs_in_Process);
  free(Results_in_Process);
}




void Do_The_Job(int my_job)
{
   long integer,i;
   double r,rt;

   if(Job_Rank ==0)
      integer=Initial_Data_Of_A_Job_Group[my_job-DMSM_Get_Group_Start(my_job)];

   MPI_Bcast(&integer, 1, MPI_LONG, 0, Job_Communicator);

   r=(double)0.0;
   for(i=(long)Job_Rank; i<=integer; i+=Job_Processes)
      {r+=sqrt((double)i);}

   rt=(double)0.0;
   MPI_Reduce(&r,&rt,1,MPI_DOUBLE,MPI_SUM,0,Job_Communicator);

   if(Job_Rank ==0)
     {Jobs_in_Process[Number_of_Results_in_Process]=my_job;
      Results_in_Process[Number_of_Results_in_Process]=rt;
      Number_of_Results_in_Process++;}
}




void Prepare_Job_Group_Data(int job_start, int job_end, int my_rank, int destination)
{int i,j,count,tag=175;
 MPI_Status anmpistatus;

 if((my_rank!=DMSM_Get_Master()) && (my_rank!=destination))
   {printf( "Error, I am neither the master nor the destination rank in Prepare_Job_Group_Data: %d %d %d.\n",
            my_rank,DMSM_Get_Master(),destination); MPI_Finalize(); exit(0);}

 count=job_end-job_start+1;

 if(destination == DMSM_Get_Master())
   {j=0;
    for(i=job_start;i<=job_end;i++)
       {Initial_Data_Of_A_Job_Group[j]=Initial_Data_For_Jobs[i];j++;}
   }
 else
   {if(my_rank == DMSM_Get_Master())
      {MPI_Send(&(Initial_Data_For_Jobs[job_start]),count,MPI_LONG,destination,tag,Dist_Communicator);}
    else
      {MPI_Recv(Initial_Data_Of_A_Job_Group,count,MPI_LONG,DMSM_Get_Master(),tag,Dist_Communicator,&anmpistatus);}
   }
}




void Collect_Some_Results(int my_rank, int from)
{  int i,tag=73,additional;
   MPI_Status anmpistatus;

   if((my_rank!=DMSM_Get_Master()) && (my_rank!=from))
     {printf( "Error, I am neither the master nor the rank to send any result in Collect_Some_Results: %d %d %d.\n",
              my_rank,DMSM_Get_Master(),from); MPI_Finalize(); exit(0);}

   if(from==DMSM_Get_Master())
     {for(i=0; i<Number_of_Results_in_Process; i++)
         {Final_Total_Jobs_In_Master[   Total_Of_Final_Results_In_Master]=   Jobs_in_Process[i];
          Final_Total_Results_In_Master[Total_Of_Final_Results_In_Master]=Results_in_Process[i];
          Total_Of_Final_Results_In_Master++;}
      Number_of_Results_in_Process=0;
      }

   else
     {
      if(my_rank==DMSM_Get_Master())
        {
         MPI_Recv(&additional,1,MPI_INT,from,tag,Dist_Communicator,&anmpistatus);
         if(additional >0)
           {
            MPI_Recv(&Final_Total_Jobs_In_Master[Total_Of_Final_Results_In_Master],
                                  additional,MPI_INT,from,tag,Dist_Communicator,&anmpistatus);
            MPI_Recv(&Final_Total_Results_In_Master[Total_Of_Final_Results_In_Master],
                                  additional,MPI_DOUBLE,from,tag,Dist_Communicator,&anmpistatus);
            Total_Of_Final_Results_In_Master+=additional;
            }
         }

      else
         {MPI_Send(&Number_of_Results_in_Process,1,MPI_INT,DMSM_Get_Master(),tag,Dist_Communicator);
          if(Number_of_Results_in_Process > 0)
            {MPI_Send(   Jobs_in_Process,Number_of_Results_in_Process,MPI_INT,   DMSM_Get_Master(),tag,Dist_Communicator);
             MPI_Send(Results_in_Process,Number_of_Results_in_Process,MPI_DOUBLE,DMSM_Get_Master(),tag,Dist_Communicator);
             Number_of_Results_in_Process=0;}
          }
     }

}




void Output_Results()
{  FILE * fout;
   int i,j,k;
   double r;
   if (My_MPI_Rank == DMSM_Get_Master())
      {/*fout=fopen("sqrtrootsumall.dat","w");
       fprintf(fout, " %d %d \n", Total_Of_Final_Results_In_Master, Total_Num_Of_Jobs);
       for(i=0;i<Total_Of_Final_Results_In_Master;i++)
          {fprintf(fout, " %d \n", Final_Total_Jobs_In_Master[i]);
           fprintf(fout, " %24.12e \n", Final_Total_Results_In_Master[i]);
           }
       fprintf(fout, " Done with example4. \n");
       fclose(fout); */

       for(i=0;i<Total_Of_Final_Results_In_Master-1;i++)
       for(j=i+1;j<Total_Of_Final_Results_In_Master;j++)
          if(Final_Total_Jobs_In_Master[i] > Final_Total_Jobs_In_Master[j])
            {k=Final_Total_Jobs_In_Master[i];
             Final_Total_Jobs_In_Master[i]=Final_Total_Jobs_In_Master[j];
             Final_Total_Jobs_In_Master[j]=k;
             r=Final_Total_Results_In_Master[i];
             Final_Total_Results_In_Master[i]=Final_Total_Results_In_Master[j];
             Final_Total_Results_In_Master[j]=r;
            }

       fout=fopen("sqrtrootsumall.dat","w");
       fprintf(fout, " %d \n", Total_Of_Final_Results_In_Master) ;
       for(i=0;i<Total_Of_Final_Results_In_Master;i++)
           fprintf(fout, " %d %24.12e \n", Final_Total_Jobs_In_Master[i], Final_Total_Results_In_Master[i]);
       fprintf(fout, " Done by dmsm.example.mpi.c. \n");
       fclose(fout);

       printf(" Done by dmsm.example.mpi.c. \n");
      }
}



