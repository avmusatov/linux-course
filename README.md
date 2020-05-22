# HSE unix course

language: C

The program is implementation of simple **daemon**, which doing something when he obtains signals.

## How to use

* **compile**: $ gcc daemon.c -o daemon -lpthread
* **run**: $ ./daemon config.txt
* **write message to log**: $ kill -SIGUSR1 pid
* **get and execute the command from "console.txt"**: $ kill -SIGUSR2 pid
* **finish**: $ kill pid

## TO DO

* ~~implement execution of several commands from single file~~
* every command running in own fork-exec parallel
* fix zombie processes