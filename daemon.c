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
#include <semaphore.h>
#include <pthread.h>

sem_t sem;

bool sigUsr1Come = false;
bool sigTermCome = false;
bool sigUsr2Come = false;
bool sigChild = false;

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

void SigChildHandler(int signum)
{
	sigChild = true;
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

int ExecuteCommand(char **newArgv)
{
	int output = open("output.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);

	dup2(output, STDOUT_FILENO);
	close(output);

	execv(newArgv[0], newArgv);
}

int Daemon(char *filePath)
{
	signal(SIGUSR1, SigUsr1Handler);
	signal(SIGUSR2, SigUsr2Handler);
	signal(SIGTERM, SigTermHandler);
	signal(SIGCHLD, SigChildHandler);

	sem_init(&sem, 0, 1);

	int cnt, status;
	int configDescriptor = open(filePath, O_RDWR, S_IRWXU);
	char allLines[64256];

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

			cnt = read(configDescriptor, allLines, sizeof(allLines));
			close(configDescriptor);
			if (cnt > 0)
			{
				char *tokens[10];
				int commandsCnt = 0;
				char *commandLine;

				commandLine = strtok(allLines, "\n");
				while (commandLine != NULL)
				{
					tokens[commandsCnt++] = commandLine;
					commandLine = strtok(NULL, "\n");
				}

				for (int k = 0; k < commandsCnt; k++)
				{
					pid_t pid;
					if ((pid = fork()) == 0)
					{
						char *newArgv[10];
						int newArgc = 0;
						char *arg;

						arg = strtok(tokens[k], " ");
						newArgv[newArgc++] = arg;

						while (arg != NULL)
						{
							arg = strtok(NULL, " ");
							newArgv[newArgc++] = arg;
						}

						newArgv[newArgc] = NULL;

						int wait = sem_wait(&sem);

						if (wait == -1)
						{
							WriteLog("Error while sem_wait operation!", 32);
						}
						else
						{
							char logMsg[100] = "Command is executed:";
							strcat(logMsg, newArgv[0]);
							WriteLog(logMsg, 21 + strlen(newArgv[0]));
							ExecuteCommand(newArgv);
						}
					}
					else if (pid > 0)
					{
						while (1)
						{
							if (sigChild)
							{
								waitpid(-1, NULL, 0);
								sem_post(&sem);
								sigChild = false;
								break;
							}
							pause();
						}
					}
					else
					{
						WriteLog("Error while fork()!", 20);
					}
				}
			}
			sigUsr2Come = false;
		}
		pause();
	}

	sem_destroy(&sem);

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

	exit(status);
}