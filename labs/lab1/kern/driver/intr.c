#include <sw.h>

void intr_enable(void) { swpipl(0); }
void intr_disable(void) { swpipl(7); }
