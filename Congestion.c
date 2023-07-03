#include "Congestion.h"

#define G 0.1
#define K 4.0
#define alpha 0.125
#define beta 0.25

// updateCWND: Update reli->cwnd according to the congestion control algorithm.
// 'reli' provides an interface to access struct Reliable.
// 'reliImpl' provides an interface to access struct ReliableImpl.
// 'acked'=true when a segment is acked.
// 'loss'=true when a segment is considered lost or should be fast retransmitted.
// 'fast'=true when a segment should be fast retransmitted.
uint32_t updateCWND(Reliable *reli, ReliableImpl *reliImpl, bool acked, bool loss, bool fast)
{
    //TODO: Your code here
}

// updateRTO: Run RTT estimation and update RTO.
//  You can use get_current_time() in Util.h/c to get current timestamp.
// 'reli' provides an interface to access struct Reliable.
// 'reliImpl' provides an interface to access struct ReliableImpl.
// 'timestamp' indicates the time when the sampled packet is sent out.
double updateRTO(Reliable *reli, ReliableImpl *reliImpl, double timestamp)
{
    //TODO: Your code here
}

#undef G
#undef K
#undef alpha
#undef beta
