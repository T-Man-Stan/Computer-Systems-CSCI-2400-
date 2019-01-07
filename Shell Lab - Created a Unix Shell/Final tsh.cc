/*
 * Trevor Stanley trst9490
 * 11.14.18
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

/* TEST CASES - passes all */

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXinput     128   /* max input on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 // undefined
#define FG 1    // running in foreground
#define BG 2    // running in background */
#define ST 3    // stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

//********************Global variables******************************************
extern char **environ;      //defined in libc
char prompt[] = "tsh> ";    //command line prompt (DO NOT CHANGE)
int verbose = 0;            // if true, print additional output
int nextjid = 1;            // next job ID to allocate
char sbuf[MAXLINE];         // for composing sprintf messages

//********************The job struct***********************
/*
struct job_t // The job struct
{
    pid_t pid;              // job PID 
    int jid;                // job ID [1, 2, ...]
    int state;              // UNDEF, BG, FG, or ST 
    char cmdline[MAXLINE];  // command line 
};
*/
//struct job_t jobs[MAXJOBS]; /* The job list */


//********************Function prototypes***************************************

//********************Here are the functions that you will implement
void eval(char *cmdline); //done
int builtin_cmd(char **argv); //done
void do_bgfg(char **argv); //done
void waitfg(pid_t pid); //done

void sigchld_handler(int sig); //done
void sigtstp_handler(int sig); //done
void sigint_handler(int sig); //done

//********************Here are helper routines that we've provided for you
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

//********************MAIN******************************************************
int main(int argc, char **argv)
{
   char c;
   char cmdline[MAXLINE];
   int emit_prompt = 1; /* emit prompt (default) */

   /* Redirect stderr to stdout (so that driver will get all output
    * on the pipe connected to stdout) */
   dup2(1, 2);

   while ((c = getopt(argc, argv, "hvp")) != EOF) //Parse the command line
   {
      switch (c)
      {
         case 'h': // print help message
            usage();
	      break;
         case 'v': //emit additional diagnostic info
            verbose = 1;
	      break;
         case 'p': //don't print a prompt
            emit_prompt = 0; //handy for automatic testing
	      break;
	      default:
            usage();
	   }
   }//end of while

   //Install the signal handlers
   //These are the ones you will need to implement
   Signal(SIGINT,  sigint_handler); //ctrl-c
   Signal(SIGTSTP, sigtstp_handler); //ctrl-z
   Signal(SIGCHLD, sigchld_handler); // Terminated or stopped child
   Signal(SIGQUIT, sigquit_handler); //This one provides a clean way to kill the shell */
   //Initialize the job list
   initjobs(jobs);

//********************Execute the shell's read/eval loop************************
   while (1)
   {
   	//Read command line
   	if (emit_prompt)
      {
   	   printf("%s", prompt);
   	   fflush(stdout);
   	}
   	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
      {
         char tempA[]="fgets error";
         app_error(tempA);
      }
   	if (feof(stdin)) //End of file (ctrl-d)
      {
   	   fflush(stdout);
   	   exit(0);
   	}
   	//Evaluate the command line
   	eval(cmdline);
   	fflush(stdout);
   	fflush(stdout);
   } //end of while
   exit(0); //control never reaches here
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
*/

/*BIG IDEA:
   Evaluates the command line that the user has typed in
   Built-in command
      eval() function returns, executes immediately (quit, jobs, bg, fg)
   No input: return
   NOT Built-in
      3 Cases
*/

//parent runs original process, child (clone) runs process interacting with user
void eval(char *cmdline) //Main route that parses & interprets command line
{
   //variable declarations
   char *argv[MAXinput];    //creates an array of chars, that is the input
   pid_t pid;               //unique program ID
   sigset_t mask;           //mask is used to block signals

   //declare and initialize a signal set
   sigemptyset(&mask);                    //initalize empty set of signals
   sigaddset(&mask, SIGCHLD);             //add a signal to a signal set
   int bg = parseline(cmdline, argv);     //parse the user input (returns 1 if background, 0 if foreground)

   //CASE 1: no inputs (empty lines)
   if (argv[0] == NULL)
   {
	  return;
   }

   //CASE 2: not a built in command
   //Note: If a built in command, executes command immediately!
   if (!builtin_cmd(argv)) //If user input is not a built in command, fork()
   {
   	sigprocmask(SIG_BLOCK, &mask, 0);
      //applications block and unblock selected signals -> changes currently blocked signals
      //specific behavior of sigprocmask() depends on input
      //SIG_BLOCK -> add signals in set to blocked
      //run so critical sections run w/out errors

      pid = fork();
      //unique PID for each fork()
      //makes child processes
      //three possible return cases for PID

      //CASE 1: PID < 0 -> ERROR!!!
      //currently parent process, child couldn't be created
   	if (pid < 0)
      {
   	   printf("%s: Forking error! \n", argv[0]);
   	   return;
   	}
      //CASE 2: PID = 0 -> currently running the NEW child process
   	else if (pid == 0)
      {
         setpgid(0,0);
         //change child process group id
         //puts the child in a new process group
         //group ID is identical to the child’s PID
         //ensures only one process, your shell, in foreground process group

         sigprocmask(SIG_UNBLOCK, &mask, 0); //parent unblocks SIGCHLD, same signal as parent, need to unblock to execute

         //checks to make sure we can overlay a new image onto old image
         //only in the case of running a new child process
         if (execv(argv[0], argv) < 0) //execv call was unsucessful
         {
            //note: successful call to execv DOES NOT have a return value
            printf("%s: Command not found \n", argv[0]);
            exit(0);
         }
   	}
      //CASE 3: PID > 0 -> currently running the parent process
      else
      {
         //Two possible cases if parent process

         //CASE 1: background (from parsed command line)
   	   if (bg == 1)
         {
      		addjob(jobs, pid, BG, cmdline);         //add job to job list as BG
            sigprocmask(SIG_UNBLOCK, &mask, 0);     //parent unblocks SIGCHLD, reap so we can terminate
      		printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
   	   }
         //CASE 2: foreground
         else
         {

   		   addjob(jobs, pid, FG, cmdline);         //add job to job list as FG
            sigprocmask(SIG_UNBLOCK, &mask, 0);     //need to unblock in order to terminate the background jobs
            waitfg(pid);                            //waits for foreground process to terminate

   	   }
   	}
   }
   return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a background (bg) job, false if
 * the user has requested a foreground (fg) job.
 */
/*
int parseline(const char *cmdline, char **argv)
{
   static char array[MAXLINE]; // holds local copy of command line
   char *buf = array;          // ptr that traverses command line
   char *delim;                // points to first space delimiter
   int argc;                   // number of input
   int bg;                     // background job?

   strcpy(buf, cmdline);
   buf[strlen(buf)-1] = ' ';     // replace trailing '\n' with space
   while (*buf && (*buf == ' ')) // ignore leading spaces
   {
      buf++;
   }
   //Build the argv list
   argc = 0;
   if (*buf == '\'')
   {
	   buf++;
	   delim = strchr(buf, '\'');
   }
   else
   {
	   delim = strchr(buf, ' ');
   }
   while (delim)
   {
	   argv[argc++] = buf;
	   *delim = '\0';
      buf = delim + 1;
	      while (*buf && (*buf == ' ')) // ignore spaces
         {
            buf++;
         }
   	if (*buf == '\'')
      {
   	   buf++;
   	   delim = strchr(buf, '\'');
   	}
   	else
      {
   	   delim = strchr(buf, ' ');
   	}
   }
   argv[argc] = NULL;
   if (argc == 0)  // ignore blank line
   {
      return 1;
   }
   //should the job run in the background?
   if ((bg = (*argv[argc-1] == '&')) != 0)
   {
	   argv[--argc] = NULL;
   }
   return bg;
}
*/


/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.*/
int builtin_cmd(char **argv) //built in commands such as quit, jobs,
{
    //char str[]="jobs";
   //strcmp returns 0 if strings are equivalent
   if (strcmp("quit", argv[0]) == 0)
   {
      exit(0);
   }
   else if (strcmp("jobs", argv[0]) == 0)
   {
      listjobs(jobs);
      return 1;
   }
   // if (cmd == "bg" || cmd == "fg"){
   // do_bgfg(argv);
   //return1;
 
   else if (strcmp("bg", argv[0]) == 0)
   {
      do_bgfg(argv);
      return 1;
   }
   else if (strcmp("fg", argv[0]) == 0)
   {
      do_bgfg(argv);
      return 1;
   }
   return 0; //not a built in command
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */

void do_bgfg(char **argv) //moves from background to foreground
{
   //variable declarations
   int jid;                      //job id, program id
   pid_t pid;
   char *input = argv[1];        //input saves string from position 1 in argv, now an array of characters
   struct job_t *job = NULL;     //make an instance of the job struct for each job called

   //First, determine if PID or JID
   if (input == NULL)            //only do fg/bg -> need to tell it what to move
   {
      printf("%s command requires PID or %%jobid argument\n", argv[0]);
      return;
   }
   //CASE 1: check first two characters -> job id
	else if (input[0] == '%' && isdigit(input[1]))
   {
	   jid = atoi(&input[1]);
      job = getjobjid(jobs, jid);

	   if (job == NULL)
      {
		   printf("%s: No such job\n", argv[1]);
		   return;
	   }
	}
   //CASE 2: check if number -> program id
   else if (isdigit(*argv[1]))
   {
	   pid = atoi(&input[0]);
      job = getjobpid(jobs, pid);
	   if (job == NULL)
      {
   		printf("(%s): No such process\n", argv[1]);
   		return;
	   }
	}
   //CASE 3: INVALID!!!
   else
   {
	   printf("%s: argument must be a PID or %%jobid\n", argv[0]);
	   return;
	}

   //Check user inputs
   //A negative PID causes the signal to be sent to every process in process group PID
   //CASE 1: user input bg
   if(strcmp("bg", argv[0]) == 0)
   {
      job->state = BG;        //change the state to bg: (STb (stopped) -> BG)
      printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);

      //send SIGCONT signal (continue process if stopped) to other process (whatever is next)
      //kill the process you are moving
      //a negative PID causes the signal to be sent to every process in process group PID
      kill(-job->pid, SIGCONT);
   }
   //CASE 2: user input fg
   else if(strcmp("fg", argv[0]) == 0)
   {
      job->state = FG;        //change the state to fg: (BG -> FG) or (ST -> FG)

      //send SIGCONT signal (continue process if stopped) to other process
      kill(-job->pid, SIGCONT);
      waitfg(job->pid); // wait for fg process to finish
   }
   return;
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)  //wait for a specific foreground process to terminate
{
   struct job_t *job;
   while (1)            //busy loop, waiting for the sleep/clean up
   {
      job = getjobpid(jobs, pid);            //which process to wait on
      if (job == NULL || (job->state != FG)) //if job is null or not in fg // (job->state != FG)
      {
         return;
      }
      sleep(150);
   }
   return;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig)
{
   int status;
   pid_t pid;
   struct job_t* jid;

   //waitpid(pid, status, options)
   //wait allows system to clean up zombie processs from terminated child
   //waitpid() system call suspends execution of the calling process
   //until a child specified (this case it doesn't matter which child)
   //by pid argument has changed state

   //waitpid() > 0:
   //Returns: PID of child if OK, 0 (if WNOHANG) or −1 on error
   //    PID of child -> child that caused waitpid to return, the one that stopped/terminated

   //in waitpid -> pid = -1: wait for any child process to terminate

   //OPTIONS: WNOHANG | WUNTRACED
   //Return value = 0
   //    None of the children in the wait set have stopped or terminated
   //Return value = PID of a terminated child
   //    Return value equal to the PID of one of the stopped or terminated children

   while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0 )
   {
   	if (WIFEXITED(status))          //checks if child terminated normally
      {
   	   deletejob(jobs,pid);         //delete entries from the job list once parent reaps child process
   	}
      else if (WIFSIGNALED(status))   //checks if child was terminated by a signal that was not caught
      {
         printf("Job [%d] (%d) terminated by signal %d \n", pid2jid(pid), pid, WTERMSIG(status));
   	   deletejob(jobs,pid);         //delete entries from the job list once parent reaps child process
   	}
   	else if (WIFSTOPPED(status))    //checks if child process that caused return is currently stopped
      {
         //WSTOPSIG returns the number of the signal that caused the child to stop
         jid = getjobpid(jobs, pid);
         if (jid == NULL)
         {
            printf("HERE\n");
            return;
         }
   	   jid->state = ST;
         printf("Job [%d] (%d) stopped by signal %d \n", pid2jid(pid), pid, WSTOPSIG(status));
   	}
   }
   // if (pid < 0 && errno != ECHILD)     //normal termination if no more children, back in parent
   // {
	//    printf("waitpid error: %s\n", strerror(errno));
   // }
   // return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)
{
   pid_t pid = fgpid(jobs);

   if (fgpid(jobs) != 0)
   {
      //A negative PID causes the signal to be sent to every process in process group PID
      //SIGINT interrupts the application, usuallly causes to abort, up to application to decide
      //ABORTS APPLICATION almost immediately
      kill(-pid, SIGINT);
   }
   return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
   pid_t pid = fgpid(jobs);

   if (pid != 0)
   {
      //A negative PID causes the signal to be sent to every process in process group PID
      //SIGTSTP to a foreground application, putting into background (suspended)
      //    Useful to break out of something like an editor to go grab some data you need
      //    Return to application by running fg
      //SHUTS INTO BACKGROUND (suspeded)
	   kill(-pid, SIGTSTP);
   }
   return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct 
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}
*/
//********************initjobs - Initialize the job list************************
/*
void initjobs(struct job_t *jobs)
{
   for (int i = 0; i < MAXJOBS; i++)
   {
      clearjob(&jobs[i]);
   }
}
*/
//********************maxjid - Returns largest allocated job ID*****************
/*
int maxjid(struct job_t *jobs)
{
   int max = 0;

   for (int i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}
*/
//********************addjob - Add a job to the job list************************
/*
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
   if (pid < 1)
   {
      return 0;
   }
   for (int i = 0; i < MAXJOBS; i++)
   {
   	if (jobs[i].pid == 0)
      {
   	   jobs[i].pid = pid;
   	   jobs[i].state = state;
   	   jobs[i].jid = nextjid++;
   	      if (nextjid > MAXJOBS)
            {
               nextjid = 1;
            }
   	   strcpy(jobs[i].cmdline, cmdline);
     	   if(verbose)
         {
   	      printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
         }
         return 1;
   	}//end of if
   }//end of for
   printf("Tried to create too many jobs\n");
   return 0;
}
*/
//********************deletejob - Delete a job whose PID=pid from the job list**
/*
int deletejob(struct job_t *jobs, pid_t pid)
{
   int i;
   if (pid < 1)
   {
      return 0;
   }
   for (i = 0; i < MAXJOBS; i++)
   {
   	if (jobs[i].pid == pid)
      {
   	   clearjob(&jobs[i]);
   	   nextjid = maxjid(jobs)+1;
   	   return 1;
   	}
   }//end of for
   return 0;
}
*/
//********************fgpid - Return PID of current foreground job**************
/*
pid_t fgpid(struct job_t *jobs) {
   int i;
   for (i = 0; i < MAXJOBS; i++)
   {
      if (jobs[i].state == FG)
      {
         return jobs[i].pid;
      }
   }
   return 0; //no current job in foreground
}
*/
//********************getjobpid  - Find a job (by PID) on the job list**********
/*
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
   int i;
   if (pid < 1)
   {
      return NULL;
   }
   for (i = 0; i < MAXJOBS; i++)
   {
      if (jobs[i].pid == pid)
      {
         return &jobs[i];
      }
   }
   return NULL;
}
*/
//********************getjobjid  - Find a job (by JID) on the job list**********
/*
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
   int i;
   if (jid < 1)
   {
      return NULL;
   }
   for (i = 0; i < MAXJOBS; i++)
   {
      if (jobs[i].jid == jid)
      {
         return &jobs[i];
      }
   }
   return NULL;
}
*/
//********************pid2jid - Map process ID to job ID************************
/*
int pid2jid(pid_t pid)
{
   if (pid < 1)
   {
      return 0;
   }
   for (int i = 0; i < MAXJOBS; i++)
   {
      if (jobs[i].pid == pid)
      {
         return jobs[i].jid;
      }
   }
   return 0;
}
*/
/* listjobs - Print the job list */
/*
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG:
		    printf("Running ");
		    break;
		case FG:
		    printf("Foreground ");
		    break;
		case ST:
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ",
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
*/
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}
*/
/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */

/*
handler_t *Signal(int signum, handler_t *handler)
{
   struct sigaction action, old_action;

    char tempA2[]= "Signal error";
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); // block sigs of type being handled //
    action.sa_flags = SA_RESTART; // restart syscalls if possible //

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error(tempA2);
    return (old_action.sa_handler);
}
*/
/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.

void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
*/
