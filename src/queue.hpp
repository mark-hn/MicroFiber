#ifndef QUEUE_HPP_
#define QUEUE_HPP_


typedef struct Thread node_item_t;
typedef struct fifo_queue fifo_queue_t;

/*
 * Checks whether the node is already part of a queue.
 * Returns true if yes, false if no.
 */
bool node_in_queue(node_item_t * node);

/*
 * Creates a new queue structure.
 * Returns a dynamically allocated queue structure upon success.
 * When out of memory, or if capacity is less than or equal to 0,
 * return NULL.
 */
fifo_queue_t * queue_create(unsigned capacity);

/*
 * Deletes an empty queue.
 * If the queue is not empty when this function is called, the program
 * should crash on an assertion error.
 */
void queue_destroy(fifo_queue_t * queue);

/*
 * Returns the node at the top of the queue and removes it from the queue,
 * or NULL if the queue is empty.
 */
node_item_t * queue_pop(fifo_queue_t * queue);

/*
 * Returns the node at the top of the queue without popping it, or NULL if the
 * queue is empty.
 */
node_item_t * queue_top(fifo_queue_t * queue);

/*
 * Insert the node to the bottom of the queue.
 * Returns 0 on success,
 *        -1 if the queue is at capacity (do not insert in this case).
 */
int queue_push(fifo_queue_t * queue, node_item_t * node);

/*
 * Returns the node in the queue with the specified id and removes it from the queue.
 * You may assume all ids in a queue are unique.
 * Returns NULL if no node in the queue has the specified id.
 */
node_item_t * queue_remove(fifo_queue_t * queue, int id);

/*
 * Returns number of items currently in queue.
 */
unsigned int queue_count(fifo_queue_t * queue);

#endif /* QUEUE_HPP_ */