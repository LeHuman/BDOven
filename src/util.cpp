#include "util.h"

char *ftoa(float num, char *buf, int sz) {
    int i = (int)num;
    int f = int((num - i) * 100);
    snprintf(buf, sz, "%i.%i", i, f);
    return buf;
}