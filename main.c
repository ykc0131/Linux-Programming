#include "smallsh.h"

char *prompt="~";

void setprompt(){
	char cwd[MAXARG]="";
	char* result;
	char dest[MAXARG]="~";
	getcwd(cwd,MAXARG);

	struct passwd* pwd = getpwuid(getuid());
	char* homedir = pwd->pw_dir;
	if((result=strstr(cwd,homedir))==NULL)
		prompt = cwd;
	else{
		strcat(dest, result+strlen(homedir));
		prompt = dest;
	}	
}

int main() {
	sigemptyset(&act1.sa_mask);
	act1.sa_handler = chld_handler;
	act1.sa_flags = SA_RESTART;
	sigaction(SIGCHLD,&act1,NULL);

	setprompt();
	while(userin(prompt) != EOF){
		procline();
		setprompt();
	}
	return 0;
}
