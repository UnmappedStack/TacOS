#pragma once
#include <io.h>

void init_pic();
void unmask_irq(int IRQ);
void mask_irq(int IRQ);
void end_of_interrupt();
