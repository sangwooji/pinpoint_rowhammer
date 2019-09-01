
# Proof-of-concept Pinpoint Rowhammer

This software is a proof-of-concept implementation of our paper "[Pinpoint Rowhammer: Suppressing Unwanted Bit Flips on Rowhammer Attacks](https://dl.acm.org/citation.cfm?id=3329811)", published in ACM Asia Conference on Computer and Communications Security (AsiaCCS) 2019. This code is implemented upon Google's "[rowhammer-test implementation](https://github.com/google/rowhammer-test)".

To run this software:

```
./make.sh
sudo ./pinpoint_rowhammer
```

## Disclaimer
This software may induce unexpected results and harm your testing environments, and you are responsible for protecting your environments. Use this software for research purpose only.

## Compatibility
This software requires root privilege to get physical addresses. Furthermore, this software is compatible with 1-rank DRAM modules that has following bank bits: 14th bit XOR 17th bit, 15th bit XOR 18th bit, 16th bit XOR 19th bit.
For other modules, modify [GetPresumedBankNumber function](https://github.com/sangwooji/pinpoint_rowhammer/blob/c87cefb95f6ea7b1e5b3ba9595cfeb511bbf5882/pinpoint_rowhammer.cc#L90-L98). A reverse engineering method for DRAM address mapping is described in Xiao et al., "[One Bit Flips, One Cloud Flops: Cross-VM Row Hammer Attacks and Privilege Escalation](https://www.usenix.org/conference/usenixsecurity16/technical-sessions/presentation/xiao)", USENIX SECURITY 2016.
