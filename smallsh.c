#include "smallsh.h"
static char inpbuf[MAXBUF];
static char tokbuf[2*MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;
static char special[] = {' ', '\t', '&',';','\n','\0'};
static sigset_t int_set;

int userin(char* p) {
	sigemptyset(&int_set);
	sigaddset(&int_set,SIGINT);
	sigprocmask(SIG_BLOCK,&int_set,NULL);
	int c, count;
	ptr = inpbuf;
	tok = tokbuf;
	printf("%s$ ", p);
	count = 0;
	while(1) {
		if ((c = getchar()) == EOF)
			return EOF;
		if (count < MAXBUF)
			inpbuf[count++] = c;
		if (c == '\n' && count < MAXBUF) {
			inpbuf[count] = '\0';
			return count;
		}
		if (c == '\n' || count >= MAXBUF) {
			printf("smallsh: input line too long\n");
			count = 0;
			printf("%s", p);
		}
	}
}

int gettok(char** outptr) {
	int type;
	*outptr = tok;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	*tok++ = *ptr;
	switch (*ptr++) {
		case '\n':
			type = EOL;
			break;
		case '&':
			type = AMPERSAND;
			break;
		case ';':
			type = SEMICOLON;
			break;
		default:
			type = ARG;
			while(inarg(*ptr))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return type;
}

int inarg (char c) {
	char *wrk;
	for (wrk = special; *wrk; wrk++) {
		if (c == *wrk)
			return 0;
	}
	return 1;
}

void procline() {
	char *arg[MAXARG + 1];
	int toktype, type;
	int narg = 0;
	for (;;) {
		switch (toktype = gettok(&arg[narg])) {
			case ARG:
				if (narg < MAXARG)
					narg++;
				break;
			case EOL:
			case SEMICOLON:
			case AMPERSAND:
				if (toktype == AMPERSAND) type = BACKGROUND;
				else type = FOREGROUND;
				if (narg != 0) {
					arg[narg] = NULL;
					runcommand(arg, type);
				}
				if (toktype == EOL) return;
				narg = 0;
				break;
		}
	}
}

int runcommand(char **cline, int where) {
	pid_t pid;
	int status;
	
	if(strcmp(*cline,"exit")==0){
		exit(0);
	}

	/*cd*/
	if(strcmp(*cline,"cd")==0){
		if(cline[2]!=NULL){
			printf("Usage: cd <dir>\n");
			return 0;
		}
		char* destdir;
		struct passwd* pwd = getpwuid(getuid());
		char* homedir = pwd->pw_dir;
	
		if(cline[1] == NULL) 	
			//cd 
			destdir = homedir;	
		else{ 			
			//cd <dir>
			char* getdir = cline[1];
			char* result;
			char dircpy[MAXARG]="";
			strcpy(dircpy,homedir);

			if((result=strstr(getdir,"~"))==NULL)
				destdir = getdir;
			else{
				strcat(dircpy,getdir+1);
				destdir = dircpy;
			}
		}

		if(chdir(destdir)==-1)
				perror(destdir);
		return 0;
	}
	


	int redi_fd;
	int cmd=0;
	int pipe_fd[2];
	int pipe_index=0;
	for(int i=0; cline[i];i++){
		/*redirection*/
		if(strcmp(cline[i],">")==0){
			if((redi_fd = open(cline[i+1], O_WRONLY | O_CREAT, 0755))==-1){
				perror(cline[i+1]);
				return -1;
			}
			cmd=1;
			cline[i]=NULL;
			break;
		}

		/*pipe*/
		if(strcmp(cline[i],"|")==0){
			pipe_index = i;
			cmd=2;
			cline[i]=NULL;
			break;

		}
	}

	switch (pid = fork()) {
		case -1:
			perror("smallsh");
			return -1;
		case 0: 		
			if(where == FOREGROUND){
				sigprocmask(SIG_UNBLOCK,&int_set,NULL);
			}
			if(cmd==1){ //redirection
				if(dup2(redi_fd,1)==-1)
					perror("redirection failed");
			} 
			else if(cmd==2){ //pipe
				if(pipe(pipe_fd)==-1){
					perror("pipe call error");
					return -1;
				}
				switch(pid = fork()){
					case -1:
						perror("pipe failed");
						return -1;
					case 0:
						dup2(pipe_fd[1],1);
						close(pipe_fd[0]);
						close(pipe_fd[1]);
						execvp(*cline,cline);
					default:
						dup2(pipe_fd[0],0);
						close(pipe_fd[0]);
						close(pipe_fd[1]);
						execvp(cline[pipe_index+1],&cline[pipe_index+1]);
				}
				break;
			}
			execvp(*cline, cline);
			perror("execvp error");
			exit(1);
	}

	if (where == BACKGROUND) {
		printf("[Process id] %d\n", pid);
		return 0;
	}
	if (waitpid(pid, &status, 0) == -1)
		return -1;
	else
		return status;
}


void chld_handler(int signo){
	int status;
	pid_t pid;
	
	while((pid = waitpid(-1,&status,WUNTRACED | WNOHANG))>0){
		if(WIFEXITED(status)){
			return ;
		}
	}
}


void int_handler(int signo){
	printf("int_handler caught\n");
	return ;
}


