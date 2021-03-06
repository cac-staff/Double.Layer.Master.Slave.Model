!    Copyright (c) 2009-2012 by High Performance Computing Virtual Laboratory
!
!    This is test example 2 of square root summation
!    for the DMSM library.
!
!    In this test, there are three input data
!    files. One is
!    TotalNumberOfOpenMPThreadsPerProcess
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
!    The comment lines of the file dmsm.f90 is
!    a little more detailed description of the
!    DMSM model and its examples.


MODULE  MY_DATA
  USE                     OMP_LIB
  INCLUDE                 'mpif.h'
  INTEGER              :: MY_MPI_RANK, TOTAL_MPI_PROCESSES, IERR
  INTEGER              :: THREADS_PER_PROCESS_EXPECTED=0
  INTEGER              :: JOB_DISTRIBUTION_PLAN
  INTEGER              :: TOTAL_JOBS, NUM_OF_JOBS_PER_GROUP
  INTEGER, ALLOCATABLE :: INITIAL_DATA_FOR_JOBS(:)
  INTEGER, ALLOCATABLE :: INITIAL_DATA_FOR_A_JOB_GROUP(:)
  DOUBLE PRECISION, ALLOCATABLE :: FINAL_RESULT_IN_MASTER(:)
  DOUBLE PRECISION, ALLOCATABLE :: FINAL_RESULT_IN_PROCESS(:)
END MODULE  MY_DATA


MODULE ALL_MODULES
  USE MY_DATA
  USE DMSM_MODULE
END MODULE  ALL_MODULES




PROGRAM A_DMSM_EXAMPLE
  USE   ALL_MODULES
  INTERFACE
      SUBROUTINE PREPARE_FOR_A_JOB_GROUP(JOB_START,JOB_END,MY_RANK,DESTINATION)
      INTEGER:: JOB_START,JOB_END,MY_RANK,DESTINATION
      END SUBROUTINE PREPARE_FOR_A_JOB_GROUP

      SUBROUTINE DO_MY_JOB(MY_JOB)
      INTEGER:: MY_JOB
      END SUBROUTINE DO_MY_JOB
  END INTERFACE

  CALL  MPIINIT
  CALL  DATA_INITIALIZE
  CALL  DMSM_ALL(THREADS_PER_PROCESS_EXPECTED,JOB_DISTRIBUTION_PLAN, &
                 TOTAL_JOBS, NUM_OF_JOBS_PER_GROUP,&
                 DO_MY_JOB, PREPARE_FOR_A_JOB_GROUP)
  CALL  FINAL_OUTPUT()
  CALL  DATA_FINALIZE
  CALL  MPI_FINALIZE(IERR)
  STOP
END PROGRAM A_DMSM_EXAMPLE




SUBROUTINE MPIINIT
  USE  ALL_MODULES
  CALL MPI_INIT( IERR )
  CALL MPI_COMM_RANK(MPI_COMM_WORLD,MY_MPI_RANK,IERR)
  CALL MPI_COMM_SIZE(MPI_COMM_WORLD,TOTAL_MPI_PROCESSES,IERR)
  RETURN
END SUBROUTINE MPIINIT




SUBROUTINE DATA_INITIALIZE
  USE ALL_MODULES
  IMPLICIT  NONE
  INTEGER   :: I

  IF(MY_MPI_RANK.EQ.DMSM_GET_MASTER()) THEN
     OPEN(11,FILE="TotalNumberOfOpenMPThreadsPerProcess")
     READ(11,*) THREADS_PER_PROCESS_EXPECTED
     CLOSE(11)
     OPEN(11,FILE="in.dat")
     READ(11,*) JOB_DISTRIBUTION_PLAN
     READ(11,*) TOTAL_JOBS, NUM_OF_JOBS_PER_GROUP
     CLOSE(11)
  END IF

  CALL MPI_BCAST(THREADS_PER_PROCESS_EXPECTED, 1,MPI_INTEGER,DMSM_GET_MASTER(),MPI_COMM_WORLD,IERR)
  CALL MPI_BCAST(JOB_DISTRIBUTION_PLAN,        1,MPI_INTEGER,DMSM_GET_MASTER(),MPI_COMM_WORLD,IERR)
  CALL MPI_BCAST(TOTAL_JOBS,                   1,MPI_INTEGER,DMSM_GET_MASTER(),MPI_COMM_WORLD,IERR)
  CALL MPI_BCAST(NUM_OF_JOBS_PER_GROUP,        1,MPI_INTEGER,DMSM_GET_MASTER(),MPI_COMM_WORLD,IERR)

  IF(MY_MPI_RANK.EQ.DMSM_GET_MASTER()) THEN
     ALLOCATE(INITIAL_DATA_FOR_JOBS(TOTAL_JOBS))
	 OPEN(11,FILE="integers.dat")
        DO I=1,TOTAL_JOBS
           READ(11,*) INITIAL_DATA_FOR_JOBS(I)
        END DO
     CLOSE(11)
  END IF

  ALLOCATE(INITIAL_DATA_FOR_A_JOB_GROUP(NUM_OF_JOBS_PER_GROUP))

  IF(MY_MPI_RANK.EQ.DMSM_GET_MASTER()) THEN
     ALLOCATE(FINAL_RESULT_IN_MASTER(TOTAL_JOBS))
  ELSE
     ALLOCATE(FINAL_RESULT_IN_MASTER(1))
  END IF
  ALLOCATE(FINAL_RESULT_IN_PROCESS(TOTAL_JOBS))
  FINAL_RESULT_IN_PROCESS=0.0D0

  RETURN
END SUBROUTINE DATA_INITIALIZE




SUBROUTINE DATA_FINALIZE
  USE ALL_MODULES
  IMPLICIT  NONE
  IF(MY_MPI_RANK.EQ.DMSM_GET_MASTER()) THEN
     IF(ALLOCATED(INITIAL_DATA_FOR_JOBS))     DEALLOCATE(INITIAL_DATA_FOR_JOBS)
  END IF
  IF(ALLOCATED(INITIAL_DATA_FOR_A_JOB_GROUP)) DEALLOCATE(INITIAL_DATA_FOR_A_JOB_GROUP)
  IF(ALLOCATED(FINAL_RESULT_IN_MASTER))       DEALLOCATE(FINAL_RESULT_IN_MASTER)
  IF(ALLOCATED(FINAL_RESULT_IN_PROCESS))      DEALLOCATE(FINAL_RESULT_IN_PROCESS)
  RETURN
END SUBROUTINE DATA_FINALIZE




SUBROUTINE DO_MY_JOB(MY_JOB)
  USE ALL_MODULES
  IMPLICIT  NONE
  INTEGER:: MY_JOB,THEINTEGER,I
  DOUBLE PRECISION :: R
  THEINTEGER= INITIAL_DATA_FOR_A_JOB_GROUP(MY_JOB-DMSM_GET_GROUP_START(MY_JOB)+1)
  CALL DMSM_UNSET_AN_INITIAL_LOCK()
  R=0.0D0
  DO I=0,THEINTEGER
     R=R+SQRT(1.0D0*I)
  END DO
  FINAL_RESULT_IN_PROCESS(MY_JOB)=R
  RETURN
END SUBROUTINE DO_MY_JOB




SUBROUTINE PREPARE_FOR_A_JOB_GROUP(JOB_START,JOB_END,MY_RANK,DESTINATION)
   USE ALL_MODULES
   IMPLICIT  NONE
   INTEGER:: MY_RANK,DESTINATION
   INTEGER:: JOB_START, JOB_END, COUNTS, TAG=175
   INTEGER:: MPISTTS(MPI_STATUS_SIZE)

   IF((MY_RANK.NE.DMSM_GET_MASTER()).AND.(MY_RANK.NE.DESTINATION)) THEN
      WRITE(*,*) "ERROR RANKS: ",MY_RANK, ", ", DMSM_GET_MASTER()," AND ",DESTINATION
      STOP
   END IF

   COUNTS=JOB_END-JOB_START+1
   IF(MY_RANK.EQ.DESTINATION) CALL DMSM_WAIT_FOR_INITIAL_LOCKS()

   IF(DESTINATION.EQ.DMSM_GET_MASTER()) THEN
      INITIAL_DATA_FOR_A_JOB_GROUP(1:COUNTS)=INITIAL_DATA_FOR_JOBS(JOB_START:JOB_END)
   ELSE
      IF(MY_RANK.EQ.DMSM_GET_MASTER()) THEN
         CALL MPI_SEND(INITIAL_DATA_FOR_JOBS(JOB_START:),COUNTS,MPI_INTEGER, &
                       DESTINATION,TAG,MPI_COMM_WORLD,IERR)
      ELSE
         CALL MPI_RECV(INITIAL_DATA_FOR_A_JOB_GROUP,  COUNTS,MPI_INTEGER, &
                       DMSM_GET_MASTER(),TAG,MPI_COMM_WORLD,MPISTTS,IERR)
      END IF
   END IF

   RETURN
END SUBROUTINE PREPARE_FOR_A_JOB_GROUP




SUBROUTINE FINAL_OUTPUT()
  USE ALL_MODULES
  IMPLICIT  NONE
  INTEGER:: I
  IF (MY_MPI_RANK .EQ. DMSM_GET_MASTER()) FINAL_RESULT_IN_MASTER=0.0D0
  CALL MPI_REDUCE(FINAL_RESULT_IN_PROCESS,FINAL_RESULT_IN_MASTER,TOTAL_JOBS,        &
                  MPI_DOUBLE_PRECISION,MPI_SUM,DMSM_GET_MASTER(),MPI_COMM_WORLD,IERR)

  IF (MY_MPI_RANK .EQ. DMSM_GET_MASTER())   THEN
      OPEN(14,FILE="sqrtrootsumall.dat")
          WRITE(14,*) TOTAL_JOBS
          DO I=1, TOTAL_JOBS
             WRITE(14,*) I, FINAL_RESULT_IN_MASTER(I)
          END DO
          WRITE(14,*) "DONE BY EXAMPLE2."
      CLOSE(14)
      WRITE(*,*) "DONE BY EXAMPLE2."
  END IF
  RETURN
END SUBROUTINE FINAL_OUTPUT



