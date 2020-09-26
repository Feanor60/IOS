#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define nan_multiplier 1000 //multiplier for conversion between microsecond and miliseconds


//strcut used to store data in shared memory
struct shared_memory{
	int A; 
	int NE;
	int NC;
	int NB;
}shared_memory;


//recieves digit stored in string and returns it as a int
int get_digit(char *str);

//returns random number from range
int random_num(int lower, int upper);

//generates immigrant processes
void imm_generator(int PI, int IG, int IT);

//runs judge process
void judge_generator(int JG, int JT, int PI);

//initialize shared memory, opens a output file and initializes semaphores
int initialize();

//handles accesing shared memory stored in a structure
void mem_access(int ne, int nc, int nb);

void IMM_enter(int i);

void judge_conf(int JT);

void IMM_register(int i);

void IMM_want_cert(int i, int IT);

void IMM_leave(int i);

int wait_for_imm();


//clean up function that destroys semaphores at exit
void do_at_exit();

//global variables
int shared_mem;
struct shared_memory *acces = NULL;
sem_t *semafor_wait_imm = NULL;
sem_t *semafor_mem_acces = NULL;
sem_t *semafor_judge_enter = NULL;
sem_t *semafor_cert_pass = NULL;
FILE *fp = NULL;

int main(int argc, char **argv){

	atexit(do_at_exit);//executes when exiting program

	if (argc < 5){
		fprintf(stderr,"Error: invlaid number of arguments\n");
		return 1;
	}

	int PI;
	int IG;
	int JG;
	int JT;
	int IT;
	char *temp;

	//load arguments

	temp = argv[1];
	PI = get_digit(temp);

	temp = argv[2];
	IG = get_digit(temp);

	temp = argv[3];
	JG = get_digit(temp);

	temp = argv[4];
	JT = get_digit(temp);

	temp = argv[5];
	IT = get_digit(temp);


	//check arguments
	if (PI <= 0){
		fprintf(stderr,"invalid argument for PI\n");
		return -1;
	}

	if (IG < 0 || IG > 2000){
		fprintf(stderr,"invalid argument for IG\n");
		return -1;
	}

	if(IG == 0)
		IG = 1;

	if (JG < 0 || JG > 2000){
		fprintf(stderr,"invalid argument for JG\n");
		return -1;
	}

	if (JT < 0 || JT > 2000){
		fprintf(stderr,"invalid argument for JT\n");
		return -1;
	}

	if(JT == 0)
		JT = 1;

	if (IT < 0 || IT > 2000){
		fprintf(stderr,"invalid argument for IT\n");
		return -1;
	}	

	if(IT == 0)
		IT = 1;

	srand(time(0)); //seed random time, depending on the clock

	if(initialize() == 1){
		return 1;
	}

	pid_t IMM = fork();
	if( IMM == 0){
		imm_generator(PI, IG, IT);
	}
	else if(IMM < 0){
		fprintf(stderr,"Error occured while forking\n");
		return 1;
	}else{
		pid_t JUDGE = fork();

		if (JUDGE == 0){//judge process

			judge_generator(JG, JT, PI);

		}
		else if (JUDGE < 0){//error in forking
			fprintf(stderr,"Error occured while forking\n");
			return 1;
		}else{ //main process
			int status_child;

			waitpid(IMM, &status_child,0); //wait for child process to finish

			waitpid(JUDGE, &status_child,0);
		}
	}


	return 0;

}

//initialize semaphores and shared memory, open output file
int initialize(){

	fp = fopen("proj2.out","w");
	setbuf(fp,NULL);

	shared_mem = shm_open("proj2_memory", O_CREAT | O_RDWR, 0664);

	ftruncate(shared_mem, sizeof(shared_memory));

	acces = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
	if(acces == MAP_FAILED){
		return 1;
	}

	if((semafor_wait_imm = sem_open("/xbubel08_IOS_proj2_semafor",  O_CREAT | O_EXCL, 0644, 1)) == SEM_FAILED){
		fprintf(stderr,"Error: error in semaphore initialization\n");
		return 1;
	}

	if((semafor_mem_acces = sem_open("/xbubel08_IOS_proj2_semafor2",  O_CREAT | O_EXCL, 0644, 1)) == SEM_FAILED){
		fprintf(stderr,"Error: error in semaphore initialization\n");
		return 1;
	}

	if((semafor_judge_enter = sem_open("/xbubel08_IOS_proj2_semafor3",  O_CREAT | O_EXCL, 0644, 1)) == SEM_FAILED){
		fprintf(stderr,"Error: error in semaphore initialization\n");
		return 1;
	}

	if((semafor_cert_pass = sem_open("/xbubel08_IOS_proj2_semafor4",  O_CREAT | O_EXCL, 0644, 0)) == SEM_FAILED){
		fprintf(stderr,"Error: error in semaphore initialization\n");
		return 1;
	}

	return 0;
}

//process for spawning immigrants
void imm_generator(int PI,int IG,int IT){

	pid_t imm, wpid;
	int status = 0;
	

	for(int i = 1; i <= PI ; i++){

		imm = fork();

		__useconds_t make_imm = random_num(0,IG) * nan_multiplier;

		usleep(make_imm); //wait for random period of time <0;IG>

		if(imm == 0){

			sem_wait(semafor_mem_acces);

			acces->A++;
			fprintf(fp,"%d		: IMM %d		: starts\n", acces->A,i);

			sem_post(semafor_mem_acces);

			IMM_enter(i);

			IMM_register(i);

			IMM_want_cert(i,IT);

			IMM_leave(i);

			exit(0);
		}
		else if(imm < 0){ 
			fprintf(stderr,"Error: couldnt fork\n");
			exit(1);
		}
	}

	while ((wpid = wait(&status)) > 0); //waits for child processes to finish
	exit(0);
}

void IMM_enter(int i){

	//check if judge in
	
	sem_wait(semafor_judge_enter);
	sem_wait(semafor_mem_acces);

	//acces memory
	mem_access(1,0,1);
	acces->A++;
	fprintf(fp,"%d		: IMM %d		: enters		: %d		: %d		: %d\n", acces->A, i, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);
	sem_post(semafor_judge_enter);
	
	//proceed
}

void IMM_register(int i){
	//wait for mutex
	sem_wait(semafor_wait_imm);
	sem_wait(semafor_mem_acces);

	acces->A++;
	mem_access(0,1,0);
	fprintf(fp,"%d		: IMM %d		: checks		: %d		: %d		: %d\n",acces->A, i, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);
	sem_post(semafor_wait_imm);
}

void IMM_want_cert(int i, int IT){

	sem_wait(semafor_cert_pass);
	sem_wait(semafor_mem_acces);

	acces->A++;
	fprintf(fp,"%d		: IMM %d		: wants	certificate	: %d		: %d		: %d\n",acces->A, i, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);
	sem_post(semafor_cert_pass);

	__useconds_t imm_enter_sleep = random_num(0,IT);
	usleep(imm_enter_sleep);

	sem_wait(semafor_mem_acces);

	acces->A++;
	fprintf(fp,"%d		: IMM %d		: got certificate	: %d		: %d		: %d\n",acces->A, i, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);

}

void IMM_leave(int i){

	sem_wait(semafor_judge_enter);
	sem_wait(semafor_mem_acces);
	
	acces->A++;
	mem_access(0,0,-1);
	fprintf(fp,"%d		: IMM %d		: leaves		: %d		: %d		: %d\n",acces->A, i, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);
	sem_post(semafor_judge_enter);
}

void mem_access(int ne, int nc, int nb){

	acces->NE += ne;
	acces->NC += nc;
	acces->NB += nb;
}

void judge_generator(int JG,int JT, int PI){

	int imm_count = 0; //total number of resolved immigrants
	int imm_resolved = 0; //number of immigrants resolved this round

	while(PI > imm_count){ //while not all of immigrants are resolved

		__useconds_t judge_enter_sleep = random_num(0,JG) * nan_multiplier;
		usleep(judge_enter_sleep); //wait random amount of time in <0;JG>
	
		sem_wait(semafor_mem_acces);

		acces->A++;
		fprintf(fp,"%d		: JUDGE		: wants to enter\n",acces->A);

		sem_post(semafor_mem_acces);
		
		sem_wait(semafor_judge_enter);
		sem_wait(semafor_mem_acces);

		acces->A++;
		fprintf(fp,"%d		: JUDGE		: enters		: %d		: %d		: %d\n",acces->A,acces->NE, acces->NC, acces->NB);

		sem_post(semafor_mem_acces);

		imm_resolved = wait_for_imm();

		imm_count += imm_resolved;

		judge_conf(JT);

		sem_post(semafor_judge_enter);
	}

	sem_wait(semafor_mem_acces);

	acces->A++;
	fprintf(fp,"%d		: JUDGE		: finishes\n", acces->A);

	sem_post(semafor_mem_acces);
	
	exit(0);
}

void judge_conf(int JT){

	sem_wait(semafor_mem_acces);

	acces->A++;
	fprintf(fp,"%d		: JUDGE		: starts confirmation	: %d		: %d		: %d\n", acces->A, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);

	__useconds_t judge_conf_sleep = random_num(0,JT) * nan_multiplier;

	usleep(judge_conf_sleep);

	sem_wait(semafor_mem_acces);

	acces->A++;
	acces->NE = 0;
	acces->NC = 0;
	fprintf(fp,"%d		: JUDGE		: ends confirmation	: %d		: %d		: %d\n", acces->A, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);
	sem_post(semafor_cert_pass); //immigrants can now get certificate

	judge_conf_sleep = random_num(0,JT) * nan_multiplier;

	usleep(judge_conf_sleep);

	sem_wait(semafor_mem_acces);

	acces->A++;
	fprintf(fp,"%d		: JUDGE		: leaves		: %d		: %d		: %d\n", acces->A, acces->NE, acces->NC, acces->NB);

	sem_post(semafor_mem_acces);
	sem_wait(semafor_cert_pass); //immigrants can no longer get certificate

}

int wait_for_imm(){

	int num_of_imm = 0;
	int all_registered = 0;
	int mssg_printed = 0;

	while(!all_registered){

		

		sem_wait(semafor_wait_imm);
		sem_wait(semafor_mem_acces);

		if(acces->NE == acces->NC){
			all_registered = 1;
		}

		sem_post(semafor_mem_acces);
		sem_post(semafor_wait_imm);

			if(!mssg_printed && all_registered == 0){
				sem_wait(semafor_mem_acces);
				
				fprintf(fp,"%d		: JUDGE		: waits for imm		: %d		: %d		: %d\n", acces->A,acces->NE,acces->NC, acces->NB);

				sem_post(semafor_mem_acces);
				mssg_printed = 1;
		}

	}

	sem_wait(semafor_mem_acces);
	num_of_imm = acces->NE;
	sem_post(semafor_mem_acces);

	return num_of_imm;
}

int random_num(int lower, int upper) 
{  
        int num = (rand() % (upper - lower + 1)) + lower;
		return num; 
} 

int get_digit(char *str){

	char c;
	unsigned int i = 0;
	int digit = 0;
	int number = 0;

	for(i = 0; i < (strlen(str)); i++)
	{
		
		c = str[i];
		if(c >= '0' && c <= '9') //to confirm it's a digit
		{
			digit = c - '0';
			number = number*10 + digit;
		}else{
			return -1;
		}
	}

	return number;
}

void do_at_exit(){

	if (fp != NULL)
		fclose(fp);

	shm_unlink("proj2_memory");

	sem_close(semafor_judge_enter);
	sem_close(semafor_mem_acces);
	sem_close(semafor_wait_imm);
	sem_close(semafor_cert_pass);

	
	sem_unlink("/xbubel08_IOS_proj2_semafor");
	sem_unlink("/xbubel08_IOS_proj2_semafor2");
	sem_unlink("/xbubel08_IOS_proj2_semafor3");
	sem_unlink("/xbubel08_IOS_proj2_semafor4");
}