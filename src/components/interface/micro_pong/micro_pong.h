#ifndef MICRO_PONG_H
#define MICRO_PONG_H

int rendezvous(int spdid);
void call(void);
void call_cs(void);
int call_buf2buf(u32_t cb, int len);
int simple_call_buf2buf(u32_t cb, int len);


#endif /* !MICRO_PONG_H */
