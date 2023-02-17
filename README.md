# CRTP Exam Project - Cappellotto Lorenzo

This repository contains the source code and the configuration needed to run the
 code developed during the CRTP course and submitted as part of the final exam.

**Disclamer: the code can only be run in Linux**

---

First compile the source code use the make command:
```
make clean all
``` 

Then, run the program by running 
```
sudo ./supervisor -m -t -p 20000 &
./client -p 20000
```