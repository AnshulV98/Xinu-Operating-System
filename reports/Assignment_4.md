## Report Assignment 4 (ansvohra)

### Questions 

#### Which future mode was used for fibonacci and why?
  * The ``` FUTURE_SHARED ``` mode of futures was used for the futures version of fibonacci as usually in mundane coding practices arrays
  or similar data structures are used, here futures are used as a data-structure to do fibonacci operations.
  * Therefore, in an iterative approach where default initial values are set to 0 and 1, the subsequent computations are done iteratively, 
  for this reason the producer and consumers are required for calculating subsequent fibonacci terms. Therefore, a one-to-many relationship 
  is required which is provided to us by the ``` FUTURE_SHARED ``` mode of futures.
