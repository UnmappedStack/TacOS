/* fireworks test, with heavy inspiration from boron's implementation:
 * https://github.com/iProgramMC/Boron/blob/b9de18168549fc4c8c53938eee105b30ce1e3802/drivers/test/source/fworktst.c */

#include <isa/cpu.h>
#include <util.h>
#include <lock.h>
#include <kprintf.h>
#include <kernel.h>
#include <framebuffer.h>
#include <tty.h>

// not implemented yet (TODO)
#define EXIT_THREAD() for (;;)

typedef struct {
    uint64_t x, y;
    int colour;
    int particles_left;
} Explosion;

int random_colour(void) {
    return (rand() + 0x808080) & 0xFFFFFF;
}

// This is... kind of bad. my kernel threads impl doesn't yet support passing arguments
// into threads on creation, so we need a global value. Which means it also is a bit locky.
static Explosion current_explodable_coords;

void particle_thread(void) {
    // get this thread's coords as starting where the parent is
    uint64_t x = current_explodable_coords.x;
    uint64_t y = current_explodable_coords.y;
    int colour = current_explodable_coords.colour;
    int64_t vel_y = -(rand() % 10); // start by going up a bit
    int64_t vel_x = rand() % 100 - 50; // go a random amount in a random direction

    while (y < kernel_info.framebuffers[0].height && y &&
           x < kernel_info.framebuffers[0].width && x) {
        framebuffer_draw_pixel(0 /* fb 0 */, x, y, colour);

        y += vel_y;
        x += vel_x;
        vel_y++;
        if (vel_y < -10) vel_y = -10;

        // i would do yield but i feel like that's *too* slow
        for (int i = 0; i < 20000; i++) PAUSE();
    }

    current_explodable_coords.particles_left++;

    EXIT_THREAD();
}

Mutex explodable_lock = {0};
void explodable_thread(void) {
    // we pick just one framebuffer for now (TODO: all framebuffers)
    mutex_acquire(&explodable_lock);
    uint64_t x = rand() % kernel_info.framebuffers[0].width;
    uint64_t y = rand() % kernel_info.framebuffers[0].height;
    int colour = random_colour();
    klogf(LOG_DEBUG, "Explodable! (%u, %u)\n", x, y);
  
    current_explodable_coords.x = x;
    current_explodable_coords.y = y;
    current_explodable_coords.colour = colour;
    current_explodable_coords.particles_left = 0;

    int num_threads = 50;
    for (int i = 0; i < num_threads; i++) {
        add_thread_to_current_processor(
             create_thread(
                 SCHED_KERNEL,        /* sched class */
                 10,                  /* nice */
                 THREAD_FLAG_PRESENT, /* flags */
                 &particle_thread
             )
        );
    }

    while (current_explodable_coords.particles_left < num_threads);

    mutex_release(&explodable_lock);
    EXIT_THREAD();
}

void fireworks_test_thread(void) {
    // every now and then, create an explodable thread
    fill_framebuffer(0, BG_DEFAULT);
    for (;;) {
        yield();
        // spawn explodable thread
        add_thread_to_current_processor(
             create_thread(
                 SCHED_KERNEL,        /* sched class */
                 15,                  /* nice */
                 THREAD_FLAG_PRESENT, /* flags */
                 &explodable_thread
             )
        );
    }
}
