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

bool signalUser1HasCome = false;
bool processIsTerminated = false;
bool signalUser2HasCome = false;

void SignalUserHandler(int signum)
{
	signalUser1HasCome = true;
}

void SignalUser2Handler(int signum)
{
	signalUser2HasCome = true;
}

void SignalTermHandler(int signum)
{
	processIsTerminated = true;
}

int Daemon(char *filePath)
{
	int logFileDescriptor = open("log-file.txt", O_CREAT | O_RDWR, S_IRWXU);
	char message[] = "Message: signal SIGUSR1 has come\n";
	char message2[] = "Message: signal SIGUSR2 has come\n";

	char greeting[] = "DAEMON has started!\n";
	char parting[] = "DAEMON has finished the work!\n";

	signal(SIGUSR1, SignalUserHandler);
	signal(SIGUSR2, SignalUser2Handler);
	signal(SIGTERM, SignalTermHandler);

	char commandToRun[64256];
	int fileWithCommandToRunDescriptor = open(filePath, O_RDWR, S_IRWXU);
	read(fileWithCommandToRunDescriptor, commandToRun, sizeof(commandToRun));

	//argv for execve
	char *newArgv[] = {NULL, NULL};
	newArgv[0] = commandToRun;

	int pid;

	dup2(fileWithCommandToRunDescriptor, STDOUT_FILENO);
	close(fileWithCommandToRunDescriptor);

	write(logFileDescriptor, greeting, sizeof(greeting));

	while (!processIsTerminated)
	{
		if (signalUser1HasCome)
		{
			write(logFileDescriptor, message, sizeof(message));
			signalUser1HasCome = false;
		}

		if (signalUser2HasCome)
		{
			write(logFileDescriptor, message2, sizeof(message2));
			pid = fork();

			if (pid == -1)
			{
				printf("An error is occurred when fork() run!\n");
				exit(EXIT_FAILURE);
			} else if (pid == 0)
			{
				execve(commandToRun, newArgv, NULL);
			}
		}
		sleep(100);
	}

	write(logFileDescriptor, parting, sizeof(parting));

	close(logFileDescriptor);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int status;
	pid_t pid;
	pid = fork();
	char buf[64256];

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
	printf("\nDemon has run with pid = %i \n", newPid);
	printf("kill -SIGUSR1 %d - send the signal to DAEMON\n", newPid);
	printf("kill -SIGUSR2 %d - send another signal to DAEMON\n", newPid);
	printf("kill %d - stop DAEMON\n", newPid);

	close(STDIN_FILENO);
	close(STDERR_FILENO);

	status = Daemon(argv[1]);

	return status;
}