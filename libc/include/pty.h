#pragma once

int openpty(int *amaster, int *aslave, char *name, void *termp, void *winp);
