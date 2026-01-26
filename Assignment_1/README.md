# 3SH3 A1 readme

## Group Name

Group - 2

## Student number and name

Johnson Ji 400499564 \
Hongliang Qi 400493278

## Assignment contribution description:

Since this assignment is not very code-intensive, so both of the members in the group write indepedently and check with each other when we are done, so technically a separate solution is submitted by each person.

## Explaination for this assignment:

Based on the given file and starting code, I add a static variable called starting_jiffies to calculate the delta jiffies and divide by HZ to calculate the total count time, which is stored as count_time. (P.S, I find there is a warning message cause by copy_to_user(usr_buf, buffer, rv), since there is no exception handle the failure, so when compling it, there is a warning message. The solution is to use if condition to brack it and return -EFAULT when fail to copy the number byte to string).
