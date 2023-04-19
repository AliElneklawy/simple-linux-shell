#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define IN_LEN 256
#define VAR_NAME_LEN 10
#define ARGS_LEN 30

void shell();
void parent_main();
void proc_exit();
void setup_environment();
void execute_shell_bultin();
void execute_command();
int chck_shll_comm();
int get_cmnd();


FILE* fp;  // pointer to the log file
char* cmnd[20]; // parse user input (user_in)
int amp_flag = 0;  // will be used to check whether the command contains & or not. 1 if & exists, 0 if not.
int proc_no = 1;  //to keep track of the terminated process number in the log file
char* echoname;  // the name of the env variable (if $ exists)
char* env_val;  //the value of the environment variable
int j;  //will be used to point to the last element in the cmnd array
int k; //will point at the index that contains '$' in the cmnd array

int main()
{
    parent_main();
}


void parent_main()
{
    signal(SIGCHLD, proc_exit);
    setup_environment();
    shell();
}

void bury_zombies()  // kill all zombie processes
{
    int stat;
    while(waitpid(-1, &stat, WNOHANG) > 0);
}

void proc_exit()
{
    fp = fopen("/home/elneklawy/Desktop/OS lab 1/log_file.txt", "a");
    fprintf(fp, " %d- Process was terminated\n", proc_no++);
    fclose(fp);
    bury_zombies();
}


void setup_environment()
{
    char path[100]; //to store the path
    chdir(getcwd(path, sizeof(path)));
    fp = fclose(fopen("/home/elneklawy/Desktop/OS lab 1/log_file.txt", "w"));  // setup the log file
}


void shell()
{
    while(1) {

        if(!get_cmnd()) continue;  //skip empty command

        if(!strcmp(cmnd[0], "exit")){
            exit(0);
        }

        if(chck_shll_comm()){
            execute_shell_bultin();
        }
        else{
            execute_command();
        }
    }
}


int chck_shll_comm()
{
    char* shell_built_ins[] = {"cd", "echo", "export", "pwd"};
    int flag = 0;
    for(int i = 0; i < 4; i++){
        if(!strcmp(cmnd[0], shell_built_ins[i])){
            flag = 1;
            break;
        }
    }
    return flag;
}


int get_cmnd()
{
    char *username = getenv("USER"); // print the username
    printf("\n@%s>> ", username);

    char* user_in[IN_LEN];  // store the input from the user
    gets(user_in);

    if(!strcmp(user_in, ""))  return 0; //empty command

    char* ptr = strtok(user_in, " ");
    int i = 0;

    while(ptr != NULL){
        cmnd[i] = ptr;
        ptr = strtok(NULL, " ");
        i++;
    }

    j = i - 1;  //point to the last element in cmnd
    chck_env_val();  //check whether the command contains $ so it can be replaced with its value

    if(!strcmp(cmnd[i - 1], "&")){
        amp_flag = 1;  // & exists
        cmnd[i - 1] = '\0';  // don't include & not to cause error in execvp
    }
    else
        cmnd[i] = '\0';

    return 1;
}


void xprt(int stat)  //stat = 1 in case of setting or 0 in case of getting
{
    if (stat == 1){

        char *y[10];
        int i, len;
        char* name;
        char* val;
        char *res[100];

        for(i = 1; i <= j; i++){      // y will be used to handle any spaces in the input                                          
            y[i-1] = cmnd[i];
        }
        y[i - 1] = '\0';

        i = 1;
        while(y[i] != NULL){
            len = strlen(y[0]);
            memset(y[0] + len, ' ', 1);
            y[len + 1] = '\0';
            i++;
        }

        char* ptr = strtok(y[0], "=");
        i = 0;

        while(ptr != NULL){  //split on '='
            res[i] = ptr;
            ptr = strtok(NULL, "=");
            i++;
        }

        name = res[0];
        val = res[1];
        setenv(name, val, 1);
    }
    else{  //replace each env var with its value in cmnd
        env_val = getenv(echoname);
        if(!env_val)    return;  // the variable entered by the user wasn't set by setenv() first
        cmnd[k] = env_val;
    }
}

void replace_env_var()
{
    char *ptr = strtok(cmnd[k], "$");  // split the command that contains '$' at the '$' sign
    char *resf [10];
    int i = 0;
    while(ptr){
        resf[i++] = ptr;
        ptr = strtok(NULL, "$");
    }

    echoname = resf[0];
    xprt(0);
}

int chck_env_val()
{
    if(cmnd[j] == NULL){  // to avoid segmentation error
        return 0;
    }

    int flag = 0;  //will be set when '$' is encountered in the command
    for(int i = 0; i <= j; i++){
        if(strstr(cmnd[i], "$")){
            flag = 1;
            k = i;  //point at the element that contains '$'
            replace_env_var(); //if a '$' sign is encoutered, the env variable will be replaced by its value in cmnd
        }
    }
    if(!flag) {k = j;  return 0;} //if '$' is not encountered, k will normally point at the last element

    return 1;
}


void execute_shell_bultin()
{
    if(!strcmp(cmnd[0], "cd")){
        if(cmnd[1] == NULL){  // if no path is given, go to /home
            chdir("/home");
            return;
        }
        chdir(cmnd[1]);
    }
    else if(!strcmp(cmnd[0], "pwd")){
        char path[100];
        printf("%s\n", getcwd(path, sizeof(path)));
    }
    else if(!strcmp(cmnd[0], "echo")){
        int i = 1;
        if(chck_env_val()){  // if env val is required just print it and return
            printf("%s", env_val);
            return;
        }

        while(cmnd[i] != '\0'){
            printf("%s ", cmnd[i]);
            i++;
        }
    }
    else if(!strcmp(cmnd[0], "export")){
        xprt(1);  //setting
    }
}


void split_ls_args()
{
    char* ptr = strtok(cmnd[k], " ");
    int i = 1;
    while(ptr){
        cmnd[i++] = ptr;
        ptr = strtok(NULL, " ");
    }
    k = i;
}


void execute_command()
{
    pid_t child_pid = fork();

    if(child_pid == -1){
        perror("Error in child process");
    }
    else if(child_pid == 0){
        if(!strcmp(cmnd[0], "ls") && (strstr(cmnd[k], " ")))  split_ls_args();  // cmnd[1] = "-l -a" --> split_ls_args() --> cmnd[1] = "-l", cmnd[2] = "-a"
        if(execvp(cmnd[0], cmnd) < 0){
            perror("execvp error");
        }
        //cmnd[0] = '\0';
    }
    else{
        if(!amp_flag){  // no & in the command
            waitpid(child_pid, NULL, 0);
        }
    }
    amp_flag = 0;
}
