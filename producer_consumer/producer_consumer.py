import threading
import time
import random

N             = 0
buffer        = []
total_items   = 0
in_ptr        = 0
out_ptr       = 0
produced_count = 0
consumed_count = 0

mutex = threading.Semaphore(1)
empty = None
full  = threading.Semaphore(0)
count_lock = threading.Lock()

done_producing = False  # flag: producers are done

def produce_item(producer_id):
    item = random.randint(1, 100)
    print(f"  [Producer-{producer_id}] Produced  item = {item}")
    time.sleep(0.5)
    return item

def insert_item(item):
    global in_ptr
    buffer[in_ptr] = item
    in_ptr = (in_ptr + 1) % N

def remove_item():
    global out_ptr
    item = buffer[out_ptr]
    out_ptr = (out_ptr + 1) % N
    return item

def consume_item(item, consumer_id):
    print(f"  [Consumer-{consumer_id}] Consumed  item = {item}")
    time.sleep(0.5)

def producer(producer_id):
    global produced_count, done_producing

    while True:
        # Reserve one production slot so total produced items never exceeds total_items.
        with count_lock:
            if produced_count >= total_items:
                break
            produced_count += 1

        item = produce_item(producer_id)

        # Acquire in this order: empty slot first, then mutex for buffer access.
        empty.acquire()    # wait(): wait if buffer is full
        mutex.acquire()    # wait(): enter critical section

        insert_item(item)
        print(f"  [Producer-{producer_id}] Inserted  item={item}  into slot={(in_ptr - 1) % N}")

        mutex.release()    # signal(): leave critical section
        full.release()     # signal(): notify consumer

    print(f"  [Producer-{producer_id}] Finished.")

def consumer(consumer_id):
    global consumed_count

    while True:
        # Timeout prevents permanent blocking when no more items will arrive.
        acquired = full.acquire(timeout=2)

        if not acquired:
            # Re-check shared counters under lock before deciding to stop.
            with count_lock:
                if consumed_count >= total_items:
                    break
            continue  # else keep waiting

        mutex.acquire()    # wait(): enter critical section

        if consumed_count >= total_items:
            mutex.release()
            # Wake another consumer that may still be waiting on full.
            full.release()
            break

        consumed_count += 1
        item = remove_item()
        print(f"  [Consumer-{consumer_id}] Removed   item={item}  from slot={(out_ptr - 1) % N}")

        mutex.release()    # signal(): leave critical section
        empty.release()    # signal(): notify producer

        consume_item(item, consumer_id)

    print(f"  [Consumer-{consumer_id}] Finished.")

def main():
    global N, buffer, total_items, empty

    print("╔══════════════════════════════════════════════════════╗")
    print("║  Producer-Consumer Problem  (Semaphore in Python)   ║")
    print("╚══════════════════════════════════════════════════════╝\n")

    N             = int(input("Enter buffer size           (N) : "))
    total_items   = int(input("Enter total items to produce    : "))
    num_producers = int(input("Enter number of producers       : "))
    num_consumers = int(input("Enter number of consumers       : "))

    # Pre-allocate fixed-size circular buffer and initialize free-slot count.
    buffer = [0] * N
    empty  = threading.Semaphore(N)

    print("\n─── Simulation Start ───────────────────────────────\n")

    producer_threads = [threading.Thread(target=producer, args=(i+1,)) for i in range(num_producers)]
    consumer_threads = [threading.Thread(target=consumer, args=(i+1,)) for i in range(num_consumers)]

    # Start all workers, then wait for a clean shutdown.
    for t in producer_threads: t.start()
    for t in consumer_threads: t.start()

    for t in producer_threads: t.join()
    for t in consumer_threads: t.join()

    print("\n─── Simulation End ──────────────────────────────────")
    print(f"  Total produced : {produced_count}")
    print(f"  Total consumed : {consumed_count}")
    print("╔══════════════════════════════════════════════════════╗")
    print("║                     All Done!                       ║")
    print("╚══════════════════════════════════════════════════════╝")

if __name__ == "__main__":
    main()