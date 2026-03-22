/*
 * Producer-Consumer Problem — Semaphore Solution in C
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// ─── Globals set by user ─────────────────────────────────────────────────────
int  N;             // buffer size
int *buffer;        // circular buffer array
int  in  = 0;       // next insert position
int  out = 0;       // next remove position
int  total_items;   // total items to produce

// ─── Semaphores ──────────────────────────────────────────────────────────────
sem_t mutex;   // binary semaphore for mutual exclusion (init = 1)
sem_t empty;   // counting semaphore: free  slots        (init = N)
sem_t full;    // counting semaphore: filled slots        (init = 0)

// ─── Shared counters (protected by mutex) ────────────────────────────────────
int produced_count = 0;
int consumed_count = 0;

// ════════════════════════════════════════════════════════════════════════════
//  Helper functions
// ════════════════════════════════════════════════════════════════════════════

int produce_item(int producer_id) {
    int item = rand() % 100 + 1;
    printf("  [Producer-%d] Produced  item = %d\n", producer_id, item);
    sleep(1);   // simulate time to produce
    return item;
}

void insert_item(int item) {
    buffer[in] = item;
    in = (in + 1) % N;
}

int remove_item() {
    int item = buffer[out];
    out = (out + 1) % N;
    return item;
}

void consume_item(int item, int consumer_id) {
    printf("  [Consumer-%d] Consumed  item = %d\n", consumer_id, item);
    sleep(1);   // simulate time to consume
}

// ════════════════════════════════════════════════════════════════════════════
//  Producer Thread
// ════════════════════════════════════════════════════════════════════════════
void *producer(void *arg) {
    int id = *((int *)arg);
    free(arg);

    while (1) {
        // ── decide whether this producer still has work ──
        sem_wait(&mutex);
        if (produced_count >= total_items) {
            sem_post(&mutex);
            break;
        }
        produced_count++;
        sem_post(&mutex);

        // ── produce outside critical section ──
        int item = produce_item(id);

        // ── SEMAPHORE LOGIC ──────────────────────────────
        sem_wait(&empty);   // wait() : wait if buffer is full
        sem_wait(&mutex);   // wait() : enter critical section

        insert_item(item);
        printf("  [Producer-%d] Inserted  item=%d  into slot=%d\n",
               id, item, (in - 1 + N) % N);

        sem_post(&mutex);   // signal() : leave critical section
        sem_post(&full);    // signal() : notify consumer: one more item ready
        // ─────────────────────────────────────────────────
    }

    printf("  [Producer-%d] Finished.\n", id);
    return NULL;
}

// ════════════════════════════════════════════════════════════════════════════
//  Consumer Thread
// ════════════════════════════════════════════════════════════════════════════
void *consumer(void *arg) {
    int id = *((int *)arg);
    free(arg);

    while (1) {
        // ── SEMAPHORE LOGIC ──────────────────────────────
        sem_wait(&full);    // wait() : wait if buffer is empty
        sem_wait(&mutex);   // wait() : enter critical section

        if (consumed_count >= total_items) {
            sem_post(&mutex);
            sem_post(&full);    // let other consumers also exit
            break;
        }
        consumed_count++;

        int item = remove_item();
        printf("  [Consumer-%d] Removed   item=%d  from slot=%d\n",
               id, item, (out - 1 + N) % N);

        sem_post(&mutex);   // signal() : leave critical section
        sem_post(&empty);   // signal() : notify producer: one slot is free
        // ─────────────────────────────────────────────────

        consume_item(item, id);
    }

    printf("  [Consumer-%d] Finished.\n", id);
    return NULL;
}

// ════════════════════════════════════════════════════════════════════════════
//  Main
// ════════════════════════════════════════════════════════════════════════════
int main() {
    int num_producers, num_consumers;

    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║   Producer-Consumer Problem  (Semaphore in C)   ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");

    printf("Enter buffer size           (N) : "); scanf("%d", &N);
    printf("Enter total items to produce    : "); scanf("%d", &total_items);
    printf("Enter number of producers       : "); scanf("%d", &num_producers);
    printf("Enter number of consumers       : "); scanf("%d", &num_consumers);

    // ── Allocate buffer ──────────────────────────────────────────────────────
    buffer = (int *)calloc(N, sizeof(int));

    // ── Initialize semaphores ────────────────────────────────────────────────
    sem_init(&mutex, 0, 1);   // mutex = 1
    sem_init(&empty, 0, N);   // empty = N  (all slots free)
    sem_init(&full,  0, 0);   // full  = 0  (no items yet)

    printf("\n─── Simulation Start ───────────────────────────────\n\n");

    // ── Create threads ───────────────────────────────────────────────────────
    pthread_t prod_threads[num_producers];
    pthread_t cons_threads[num_consumers];

    for (int i = 0; i < num_producers; i++) {
        int *id = malloc(sizeof(int)); *id = i + 1;
        pthread_create(&prod_threads[i], NULL, producer, id);
    }
    for (int i = 0; i < num_consumers; i++) {
        int *id = malloc(sizeof(int)); *id = i + 1;
        pthread_create(&cons_threads[i], NULL, consumer, id);
    }

    // ── Wait for all threads ─────────────────────────────────────────────────
    for (int i = 0; i < num_producers; i++)
        pthread_join(prod_threads[i], NULL);

    for (int i = 0; i < num_consumers; i++)
        pthread_join(cons_threads[i], NULL);

    // ── Cleanup ──────────────────────────────────────────────────────────────
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    free(buffer);

    printf("\n─── Simulation End ─────────────────────────────────\n");
    printf("  Total produced : %d\n", produced_count);
    printf("  Total consumed : %d\n", consumed_count);
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║                    All Done!                    ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");

    return 0;
}
