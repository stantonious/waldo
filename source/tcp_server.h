#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#define BATCH_SIZE 32

typedef struct {
    uint32_t xy_step;
    uint32_t yz_step;
    float x;
    float y;
    float z;
}measurement;

typedef measurement measurement_batch[BATCH_SIZE];

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void tcp_server_task(void *arg);

cy_rslt_t init_wifi();
cy_rslt_t create_tcp_task();

#endif /* TCP_SERVER_H_ */
