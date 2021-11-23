# Relates to the ongoing XDP/eBPF project at FIU
This repo is comprised of basically two parts:

-	**part 1** deals with creating an assessment package that a DTN operator
 	can use to see what recommendations, if any, can be made based on the 
	current settings of the system.

- 	**part 2** is a superset of part 1 and has 4 phases:
	* Starting
	* Assessment (most of part 1)
	* Learning
	* Tuning

###Each part is independent of the other

**Part 1**

There are no known dependencies at this point.  There are two relevant 
directories here:

```assess-tuning``` and ```packaging```
To Compile:
-	go to assess-tuning directory and run ```make```
-	run ```sudo ./dtnmenu``` to run with a menu interaction
- 	run ```sudo ./dtn_tune``` to run without menu interaction


**Part 2**


  

* test_dtn.c refers to a test module used to create user_dtn.c
* user_dtn.c is the actual module that will eventually run as the Tuning Module
* dtn_tune.c refers to the new stuff which only features the assessment portion
