#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<X11/Xlib.h>
#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH				100

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Block;
void sighandler(int num);
void replace(char *str, char old, char new);
void remove_all(char *str, char to_remove);
void getcmds(int time);
void getsigcmds(int signal);
void setupsignals();
void getstatus(char *str);
void setroot();
void statusloop();
void statusinit();
void sighandler(int signum);
void termhandler(int signum);


#include "config.h"

static Display *dpy;
static int screen;
static Window root;
static char statusbar[LENGTH(blocks)][50] = {0};
static char statusstr[256];
static int statusContinue = 1;

void replace(char *str, char old, char new)
{
	int N = strlen(str);
	for(int i = 0; i < N; i++)
		if(str[i] == old)
			str[i] = new;
}

void remove_all(char *str, char to_remove) {
	char *read = str;
	char *write = str;
	while (*read) {
		if (*read == to_remove) {
			read++;
			*write = *read;
		}
		read++;
		write++;
	}
}

//opens process *cmd and stores output in *output
void getcmd(const Block *block, char *output)
{
	strcpy(output, block->icon);
	char *cmd = block->command;
	FILE *cmdf = popen(cmd,"r");
	if (!cmdf)
		return;
	//int N = strlen(output);
	char c;
	int i = strlen(block->icon);
	fgets(output+i, CMDLENGTH-(strlen(delim)+1), cmdf);
	remove_all(output, '\n');
	i = strlen(output);
	/* while((c = fgetc(cmdf)) != EOF) */
	/* 	output[i++] = c; */

    if ((i > 0 && block != &blocks[LENGTH(blocks) - 1]))
        strcat(output, delim);
	i += strlen(delim);
	output[i++] = '\0';
	pclose(cmdf);
}

void getcmds(int time)
{
	const Block* current;
	for(int i = 0; i < LENGTH(blocks); i++)
	{
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1)
			getcmd(current,statusbar[i]);
	}
}

void getsigcmds(int signal)
{
	const Block *current;
	for (int i = 0; i < LENGTH(blocks); i++)
	{
		current = blocks + i;
		if (current->signal == signal)
			getcmd(current,statusbar[i]);
	}
}

void setupsignals()
{
	for(int i = 0; i < LENGTH(blocks); i++)
	{
		if (blocks[i].signal > 0)
			signal(SIGRTMIN+blocks[i].signal, sighandler);
	}

}

void getstatus(char *str)
{
	int j = 0;
	for(int i = 0; i < LENGTH(blocks); j+=strlen(statusbar[i++]))
	{
		strcpy(str + j, statusbar[i]);
        if (i == LENGTH(blocks) - 1)
            strcat(str, " ");
	}
	str[strlen(str) - 1] = '\0';
}

void setroot()
{
	Display *d = XOpenDisplay(NULL);
	if (d) {
		dpy = d;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	getstatus(statusstr);
	XStoreName(dpy, root, statusstr);
	XCloseDisplay(dpy);
}


void statusloop()
{
	setupsignals();
	int i = 0;
	getcmds(-1);
	while(statusContinue)
	{
		getcmds(i);
		setroot();
		sleep(1.0);
		i++;
	}
}

void statusinit()
{
	statusloop();
}


void sighandler(int signum)
{
	getsigcmds(signum-SIGRTMIN);
	setroot();
}

void termhandler(int signum)
{
	statusContinue = 0;
	exit(0);
}

int main(int argc, char** argv)
{
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusinit();
}
