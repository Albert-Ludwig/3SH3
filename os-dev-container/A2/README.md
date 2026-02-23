# 3SH3 A2 README

## Group Name

Group - 2

## Student number and name

Johnson Ji 400499564 \
Hongliang Qi 400493278

## Assignment contribution description:

For the A2, I, student Johnson Ji design the logic of the code and implement it and student Hongliang Qi test the code and write the makefile.

## Explaination for this assignment:

This assignment implements the classic Sleeping Teaching Assistant (TA) synchronization problem using POSIX threads, mutexes, and semaphores in C.
The program includes:

- 1 TA thread
- N student threads
- A waiting queue with 3 chairs.

Each student repeat:

1. Programs for a random amount of time.
2. Tries to get help from the TA.
3. If a chair is available, joins the waiting queue.
4. If all chairs are full, returns to programming and tries again later.

The TA repeatedly:

- Sleeps when no students are waiting (blocks on waiting_student_sem).
- Wakes up when a student arrives.
- Calls the next student from the queue (FIFO behavior).
- Helps that student for a random time.
- Signals when help is finished.

Synchronization Design

- mutex: protects shared queue data and waiting count.
- waiting_student_sem: signals that at least one student is waiting.
- student_called_sem[i]: wakes a specific student when called by TA.
- help_done_sem[i]: notifies that student that help is complete.
