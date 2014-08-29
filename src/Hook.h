/*

Decoda
Copyright (C) 2007-2013 Unknown Worlds Entertainment, Inc. 

This file is part of Decoda.

Decoda is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Decoda is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Decoda.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
* modifyed by huangweilook@21cn.com 2014-08-28  
*/

#ifndef HOOK_H
#define HOOK_H

#include <assert.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>

/**
 * Returns true if the specified function is already hooked. Functions
 * cannot be rehooked to the same hook, so this should be tested before
 * calling HookFunction if there is any danger of that happening.
 */
int GetIsHooked(void* function, void* hook);

/**
 * Installs a hook function that will be called instead of the original
 * function. The function returns a pointer to a new function that can
 * be used to call the original function (or NULL if there was an error).
 */
void* HookFunction(void* function, void* hook,void *saveaddr,size_t saveaddr_size);

/**
 * Installs a hook function that will be called instead of the original
 * function. The function returns a pointer to a new function that can
 * be used to call the original function (or NULL if there was an error).
 * The up value will be pushed onto the stack prior right before the hook
 * function is called and it's the reponsibility of the hook function
 * to remove that value before it returns.
 */
//void* HookFunction(void* function, void* hook, unsigned long upValue);

/**
 */
//void* InstanceFunction(void* function, unsigned long upValue);

#endif
