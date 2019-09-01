
# Proof-of-concept Pinpoint Rowhammer

This software is a proof-of-concept implementation of our paper "[Pinpoint Rowhammer: Suppressing Unwanted Bit Flips on Rowhammer Attacks](https://dl.acm.org/citation.cfm?id=3329811)", published in ACM Asia Conference on Computer and Communications Security (AsiaCCS) 2019. This code is implemented upon Google's "[rowhammer-test implementation](https://github.com/google/rowhammer-teist)".

To run this software:

```
./make.sh
sudo ./pinpoint_rowhammer
```

## Disclaimer
This software may induce unexpected results and harm your testing environments, and you are responsible for protecting your environments. Use this software for research purpose only.
