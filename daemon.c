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

#define SEM_NAME "/semname"

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

int ExecuteCommand(char **newArgv)
{
	int output = open("output.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);

	dup2(output, STDOUT_FILENO);
	close(output);

	int pid = fork();
	int status;
	switch (pid)
	{
	case -1:
		WriteLog("An error is occurred when fork() run!", 38);
		return -1;
	case 0:
		execve(newArgv[0], newArgv, __environ);
	default:
	{
		waitpid(pid, &status, 0);
	}
	}
	return WEXITSTATUS(status);
}

int Daemon(char *filePath)
{
	signal(SIGUSR1, SigUsr1Handler);
	signal(SIGUSR2, SigUsr2Handler);
	signal(SIGTERM, SigTermHandler);

	// sem_t *sem;

	// if ((sem = sem_open(SEM_NAME, O_CREAT, 0777, 0)) == SEM_FAILED)
	// {
	// 	WriteLog("An error is occured while opening semaphore", 44);
	// 	exit(EXIT_FAILURE);
	// }

	// sem_init(sem, 1, 0);

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

			// while ((cnt = read(configDescriptor, commandLine, sizeof(commandLine))) > 0)
			// {
			// 	int ppid = fork();

			// 	if (ppid == -1)
			// 	{
			// 		WriteLog("An error is occurred when fork() run!", 38);
			// 		exit(EXIT_FAILURE);
			// 	}
			// 	else if (ppid == 0)
			// 	{
			// 		int newArgc = 0;
			// 		char *newArgv[10];
			// 		char *arg;

			// 		//parsing of the line
			// 		arg = strtok(commandLine, " ");
			// 		newArgv[newArgc++] = arg;

			// 		while (arg != NULL)
			// 		{
			// 			arg = strtok(NULL, " ");
			// 			newArgv[newArgc++] = arg;
			// 		}

			// 		newArgv[newArgc] = NULL;

			// 		dup2(outputDescriptor, STDOUT_FILENO);
			// 		close(outputDescriptor);

			// 		int pid = fork();
			// 		if (pid == -1)
			// 		{
			// 			WriteLog("An error is occurred when fork() run!", 38);
			// 			exit(EXIT_FAILURE);
			// 		}
			// 		else if (pid == 0)
			// 		{
			// 			sem_wait(sem);
			// 			execve(newArgv[0], newArgv, NULL);
			// 		}

			// 		//wait(&status);

			// 		if (status == 0)
			// 		{
			// 			WriteLog("0", 1);
			// 		}
			// 		else
			// 		{
			// 			WriteLog("1", 1);
			// 		}

			// 		sem_post(sem);
			// 		sem_close(sem);
			// 	}
			//}

			cnt = read(configDescriptor, allLines, sizeof(allLines));
			if (cnt > 0)
			{
				char *tokens[10];
				int i = 0;
				char *commandLine;
				commandLine = strtok(allLines, "\n");
				while (commandLine != NULL)
				{
					tokens[i++] = commandLine;
					commandLine = strtok(NULL, "\n");
				}

				for (int k = 0; k < i; k++)
				{
					char *newArgv[10];
					int newArgc = 0;
					char *arg;

					arg = strtok(tokens[k], " ");
					newArgv[newArgc++] = arg;

					while (arg != NULL)
					{
						//printf("[%s]\n", arg);
						arg = strtok(NULL, " ");
						newArgv[newArgc++] = arg;
					}

					newArgv[newArgc] = NULL;

					int st = ExecuteCommand(newArgv);
					if (st == 0)
					{
						WriteLog("Success", 8);
					}
					else
					{
						WriteLog("Failed", 7);
					}
				}
			}
			sigUsr2Come = false;
		}
		pause();
	}

	close(configDescriptor);
	//sem_unlink(SEM_NAME);

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