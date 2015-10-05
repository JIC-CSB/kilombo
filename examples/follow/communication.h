void rxbuffer_push(message_t *msg, distance_measurement_t *dist);
message_t *message_tx();
void process_message ();
void purgeNeighbors(void);
void setup_message(void);

#define CUTOFF 200 
