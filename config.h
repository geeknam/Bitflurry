#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CONFIG_FILE "bitflurry.conf"

// get a value in config file
void cfg_getValueForKey(char *key, char *val);

// set a value in config file
int cfg_setValueForKey(char *key, char *val);

#endif