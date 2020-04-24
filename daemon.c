#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool sigUsr1Come = false;
bool sigTermCome = false;
bool sigUsr2Come = false;

void SigUsr1Handler(int signum)
{
	sigUsr1Come = true;
}

void SigUsr2Handler(int signum)
{
	sigUsr2Come = true;
}

void SigTermHandler(int signum)
{
	sigTermCome = true;
}

void WriteLog(char *msg, int size)
{
	int logDescriptor = open("log.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	char fullMsg[64256];
	char *helper = "[DAEMON]: ";

	strcpy(fullMsg, helper);
	strcat(fullMsg, msg);
	strcat(fullMsg, "\n");

	write(logDescriptor, fullMsg, size + 11);

	close(logDescriptor);
}

int Daemon(char *filePath)
{
	signal(SIGUSR1, SigUsr1Handler);
	signal(SIGUSR2, SigUsr2Handler);
	signal(SIGTERM, SigTermHandler);

	char commandLine[64256];
	int configDescriptor = open(filePath, O_RDWR, S_IRWXU);
	read(configDescriptor, commandLine, sizeof(commandLine));

	//argv and argc for execve
	int newArgc = 0;
	char *newArgv[10];
	char *arg;

	//parsing of the line
	arg = strtok(commandLine, " ");
	newArgv[newArgc++] = arg;

	while (arg != NULL)
	{
		arg = strtok(NULL, " ");
		newArgv[newArgc++] = arg;
	}

	newArgv[newArgc] = NULL;

	int pid;

	dup2(configDescriptor, STDOUT_FILENO);
	close(configDescriptor);

	WriteLog("started!", 9);

	while (!sigTermCome)
	{
		if (sigUsr1Come)
		{
			WriteLog("SIGUSR1 come", 13);
			sigUsr1Come = false;
		}

		if (sigUsr2Come)
		{
			WriteLog("SIGUSR2 come", 13);
			pid = fork();

			if (pid == -1)
			{
				printf("An error is occurred when fork() run!\n");
				exit(EXIT_FAILURE);
			}
			else if (pid == 0)
			{
				execve(newArgv[0], newArgv, NULL);
			}
			sigUsr2Come = false;
		}
		pause();
	}

	WriteLog("finished!", 10);

	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	pid_t pid;
	pid = fork();

	if (pid == -1)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	setsid();
	pid = fork();

	if (pid == -1)
	{
		printf("An error is occurred when fork() run!\n");
		exit(EXIT_FAILURE);
	}

	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	umask(0);
	pid_t newPid = getpid();
	printf("\n DAEMON pid is %i \n", newPid);

	close(STDIN_FILENO);
	close(STDERR_FILENO);

	int status = Daemon(argv[1]);

	return status;
}