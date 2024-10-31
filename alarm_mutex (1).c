/*
 * alarm_mutex.c
 *
 * This is an enhancement to the alarm_thread.c program, which
 * created an "alarm thread" for each alarm command. This new
 * version uses a single alarm thread, which reads the next
 * entry in a list. The main thread places new requests onto the
 * list, in order of absolute expiration time. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[64];
    int                 Alarm_ID;//Ryan: ADDED for ID
    char                Type[10];//Ryan: ADDED for Type
    pthread_t           display_thread;
    int                 alarm_status;   //0 started 1 changed 2 cancelled 3 expiry_time
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;

//tester to see the entire list
void print_alarm_list() {
    alarm_t *current = alarm_list;
    if (current == NULL) {
        printf("No alarms in the list\n");
        return;
    }
    printf("Current Alarms in the List:\n");
    while (current != NULL) {
        printf("Alarm_ID: %d, Type: %s, Seconds: %d, Message: %s, Time: %ld\n", 
               current->Alarm_ID, current->Type, current->seconds, current->message, current->time);
        current = current->link; // Move to the next alarm in the list
    }
}

/*
 * The alarm thread's start routine.
 */
/*void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    int sleep_time;
    time_t now;
    int status;


    while (1) {
        status = pthread_mutex_lock (&alarm_mutex);//to lock
        if (status != 0)//if status is not locked, which status will not equal to 1, then print error message
            err_abort (status, "Lock mutex");//the system would print the error message and then terminate
        alarm = alarm_list;//the thread assigns the first alarm from the alarm_list to the alarm pointer. so pointer(alarm) is pointing to the 
                        //current alarm to be processed


        if (alarm == NULL)//if there is no alarm in the list
            sleep_time = 1;//sleep for 1 second
        else {
            alarm_list = alarm->link;//the current alarm is removed and the head of the list is pointing to next alarm in the list
            now = time (NULL);//record current time
            if (alarm->time <= now)//if the alarm's trigger time has already passed or is due
                sleep_time = 0;//don't sleep, run it immediately 
            else
                sleep_time = alarm->time - now;//if the alarm is not passed or due, then the thread get sleep_time
#ifdef DEBUG//if the program is in debug mode, this block will print debugging info like the alarm's time and the calculated sleep time and message
            printf ("[waiting: %d(%d)\"%s\"]\n", alarm->time,
                sleep_time, alarm->message);
#endif
            }


        status = pthread_mutex_unlock (&alarm_mutex);//once the thread finished working with the alarm list, it unlocks the mutex
        if (status != 0)//see if the unlock is working or not
            err_abort (status, "Unlock mutex");//system print an error message and then terminate the program
        if (sleep_time > 0)
            sleep (sleep_time);
        else//sleep_time == 0
            sched_yield ();//there is no time to sleep and sched_yield() can ask another thread to run immediately 


        if (alarm != NULL) {//print the alarm message and free the memory 
            printf ("(%d) %s\n", alarm->seconds, alarm->message);
            free (alarm);
        }
    }
}*/
void *alarm_thread(void *arg)
{
    int sleep_time;    
    time_t now;  
    int status;

    while (1) {
        // Lock the mutex to safely access the shared alarm list
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Lock mutex");

        if (alarm_list == NULL) {
            // If the alarm list is empty, set sleep_time to 1 second
            sleep_time = 1;
        } else {
            // Calculate the time until the next alarm is due
            now = time(NULL);  // Get the current time
            sleep_time = alarm_list->time - now;  // Time until the next alarm
            if (sleep_time <= 0)
                sleep_time = 0;  // if the alarm is due or overdue, set sleep_time to 0
        }

        // Unlock the mutex before sleeping to allow other threads to access the alarm list
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Unlock mutex");

        // Sleep for the calculated time
        if (sleep_time > 0)
            sleep(sleep_time);
        else
            sched_yield();  

        // After waking up, lock the mutex again to check for due alarms
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Lock mutex");

        now = time(NULL); // Update the current time

        // Process all alarms that are due
        while (alarm_list != NULL && alarm_list->time <= now) {
            // Remove the alarm from the list and process it
            alarm_t *alarm = alarm_list; // Get the first alarm in the list
            //alarm_list = alarm_list->link; // Remove it from the list
            alarm_list->alarm_status = 3;
            
            // Unlock the mutex before processing the alarm
            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock mutex");

            // Print the alarm message
            printf("(%d) %s\n", alarm->Alarm_ID, alarm->message);

            // Free the memory allocated for the alarm
            free(alarm);

            // Re-lock the mutex to check if there are more due alarms
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            now = time(NULL); // Update the current time
        }

        // Unlock the mutex after processing due alarms
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Unlock mutex");
    }
}

void *display_thread (void *arg)
{
    alarm_t *alarm, **last, *next;
    int sleep_time;
    time_t now;
    time_t last_display;
    int count;
    int status;


    while (1) {
        status = pthread_mutex_lock (&alarm_mutex);//to lock
        if (status != 0)//if status is not locked, which status will not equal to 1, then print error message
            err_abort (status, "Lock mutex");//the system would print the error message and then terminate
            alarm = alarm_list;//the thread assigns the first alarm from the alarm_list to the alarm pointer. so pointer(alarm) is pointing to the 
                        //current alarm to be processed


        if (alarm == NULL){ //if there is no alarm in the list
            //end
        } else {
            count = 0;
            last = &alarm;
            next = *last;
                while (next != NULL) {//go through the linked list of alarms
                    if(next->display_thread == pthread_self()){
                        count++;
                        now = time (NULL);//record current time
                        if (next->alarm_status == 3) {
                            //print “Alarm( <alarm_id>) Expired; Display Thread (<thread-id>) Stopped Printing AlarmMessage at <stop_display_time>: <type time message>”.
                            next->alarm_status = 4;
                             //remove
                        }
                        else if (next->alarm_status == 2) {
                           // “Alarm( <alarm_id>) Cancelled; Display Thread (<thread-id>) Stopped PrintingAlarm Message at <stop_display_time>: <type time message>”. 
                           next->alarm_status = 4;
                            //remove           
                        }
                        else if (next->alarm_status == 1) {
                           // “Alarm( <alarm_id>) Changed Type; Display Thread (<thread-id>) Stopped PrintingAlarm Message at <stop_display_time>: <type time message>”.
                           next->alarm_status = 4;
                           //remove
                        }
                        else if (next->alarm_status == 0) 
                        {
                           if(now - last_display == 5)
                           {
                              // “Alarm( <alarm_id>) Message PERODICALLY PRINTED BY Display Thread(<thread-id>) at <periodic_print_time>: <type time message>”.
                                last_display = now;
                           }     
                        }
                        else{
                            
                        }
                    }
                    
                    last = &next->link;
                    next = next->link;
                }
             if(count == 0){
                //end
             }   
            }
        status = pthread_mutex_unlock (&alarm_mutex);//once the thread finished working with the alarm list, it unlocks the mutex
    }
}

int main (int argc, char *argv[])
{
    int status;//returned value of thread-realted and mutex functions like pthread_create() and pthread_mutex_lock(), to check success or not
    char line[128];//user input
    alarm_t *alarm, **last, *next;//*alarm: pointer to alarm_t; **last: point to the next alarm's pointer; *next: insert the new alarm in the correct place
    pthread_t thread;//thread identifier that will be used to create and managed the alarm thread
    int display_thread_count = 2;
    pthread_t current_display_thread;
    status = pthread_create (//create a new thread to run the alarm_thread function
        &thread, NULL, alarm_thread, NULL);//&thread: pointer to the pthread_t object where the thread Id is stored; alarm_thread: the function to be executed by the new thread
    if (status != 0)//if it is success, then status = 0
        err_abort (status, "Create alarm thread");//error message
    while (1) {
        printf ("alarm> ");//display a prompt "alarm>" to user and asking them to enter an alarm
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);//read the info that user entered
        if (strlen (line) <= 1) continue;//double check if the user enter empty line or just press enter. if it is this case, then reprompt, dont create an alarm

        

        alarm = (alarm_t*)malloc (sizeof (alarm_t));//allocates memory for a new alarm_t with size of alarm_t
        if (alarm == NULL)//check if malloc(memory allocation) was successful
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */

        //handle start_alarm case
        if (strncmp(line, "Start_Alarm", 11) == 0) {           
            //check if user enter Alarm_ID, Type,seconds, message
            if (sscanf (line, "Start_Alarm(%d): %s %d %64[^\n]", &alarm->Alarm_ID, alarm->Type, &alarm->seconds, alarm->message) < 4) {//check if user enter both valid number of seconds and a message, if user didnt provide both, then it is bad
                fprintf (stderr, "Bad command\n");
                print_alarm_list(); // print the list(just for debugging)
                free (alarm);
            } else {
                status = pthread_mutex_lock (&alarm_mutex);//lock alarm_mutex and no other thread can modify the alarm_list while we insert new alarm
                if (status != 0)//success lock or not
                    err_abort (status, "Lock mutex");
                alarm->time = time (NULL) + alarm->seconds;//alarm->time calculate the current time(since Unix Epoch) + the user entered time
                alarm->alarm_status = 0;
                if(display_thread_count == 0){
                    status = pthread_create (//create a new thread to run the alarm_thread function
                        &current_display_thread, NULL, display_thread, NULL);//&thread: pointer to the pthread_t object where the thread Id is stored; alarm_thread: the function to be executed by the new thread
                    if (status != 0)//if it is success, then status = 0
                    err_abort (status, "Create alarm thread");//error message
                    display_thread_count = 1;
                    //print “Additional New Display Thread(<thread_id> Created at <creation_time>: <type timemessage>”.                  
                }else if(display_thread_count == 2){
                    status = pthread_create (//create a new thread to run the alarm_thread function
                        &current_display_thread, NULL, display_thread, NULL);//&thread: pointer to the pthread_t object where the thread Id is stored; alarm_thread: the function to be executed by the new thread
                    if (status != 0)//if it is success, then status = 0
                    err_abort (status, "Create alarm thread");//error message
                    display_thread_count = 1;
                    //print “First New Display Thread(<thread_id>) Created at <creation_time>: <type timemessage>”.    
                }
                else
                    display_thread_count = 0;
                }
                alarm->display_thread = current_display_thread;
                //print “Alarm (<alarm-id>) Assigned to Display Thread(<thread_id>) at <assign_time>:<type time message>”.
                /*
                * Insert the new alarm into the list of alarms,
                * sorted by expiration time.
                */
                last = &alarm_list;
                next = *last;
                while (next != NULL) {//go through the linked list of alarms
                    if ((next->Alarm_ID > alarm->Alarm_ID) || (next->Alarm_ID == alarm->Alarm_ID && next->time >= alarm->time)) {//if the current alarm trigger time is greater than or equal to the new alarms' time, then this is a correct postion to insert. Ryan: also added the ID check
                        alarm->link = next;
                        *last = alarm;
                        break;
                    }
                    last = &next->link;
                    next = next->link;
                }
                /*
                * If we reached the end of the list, insert the new
                * alarm there. ("next" is NULL, and "last" points
                * to the link field of the last item, or to the
                * list header).
                */
                if (next == NULL) {//if we reach the end of the list
                    *last = alarm;
                    alarm->link = NULL;
                }
                printf("Alarm(%d) Inserted by Main Thread(%ld) Into Alarm List at %ld: %s %d %s\n", alarm->Alarm_ID, (unsigned long)pthread_self(), time(NULL), alarm->Type, alarm->seconds, alarm->message);//print the output
                print_alarm_list(); // print the list(just for debugging)

    #ifdef DEBUG//if we are in debug mode, then this will print the current state of the alarm list, with each alarm's trigger time and message
                printf ("[list: ");
                for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time,
                        next->time - time (NULL), next->message);
                printf ("]\n");
    #endif
                status = pthread_mutex_unlock (&alarm_mutex);//after insert the new alarm into the list, unlocked mutex, allow to access this list again
                if (status != 0)
                    err_abort (status, "Unlock mutex");
            }
        else if (strncmp(line, "Change_Alarm", 12) == 0) {
            int alarm_id;
            char new_type[10];
            int new_seconds;
            char new_message[64];

            if (sscanf(line, "Change_Alarm(%d): %s %d %63[^\n]", &alarm_id, new_type, &new_seconds, new_message) < 4) {
                fprintf(stderr, "Bad Change_Alarm command\n");
                print_alarm_list(); // Just for debugging
                continue; // Skip to the next input
            } else {
                printf("Parsed Change_Alarm: ID = %d, Type = %s, Seconds = %d, Message = %s\n", 
                        alarm_id, new_type, new_seconds, new_message);
            }

            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0) err_abort(status, "Lock mutex");

            last = &alarm_list;
            next = *last;
            print_alarm_list(); // Debugging before change

            while (next != NULL) {
                printf("Comparing Alarm_ID in List: %d with input Alarm_ID: %d\n", next->Alarm_ID, alarm_id); // Debugging

                if (next->Alarm_ID == alarm_id) {
                    printf("Found Alarm(%d) in list. Proceeding to change...\n", next->Alarm_ID);
                    next->alarm_status = 1;
                    *last = next->link;

                    strcpy(next->Type, new_type);
                    next->seconds = new_seconds;
                    next->time = time(NULL) + new_seconds;
                    strcpy(next->message, new_message);

                    alarm_t *insert_alarm = next;
                    last = &alarm_list;
                    next = *last;

                    while (next != NULL) {
                        if (insert_alarm->time < next->time) {
                            insert_alarm->link = next;
                            insert_alarm->alarm_status = 0;
                            *last = insert_alarm;                            
                            break;
                        }
                        last = &next->link;
                        next = next->link;
                    }
                    if (next == NULL) {
                        *last = insert_alarm;
                        insert_alarm->link = NULL;
                    }

                    printf("Alarm(%d) Changed by Main Thread(%ld) at %ld: %s %d %s\n",insert_alarm->Alarm_ID, (unsigned long)pthread_self(), time(NULL), insert_alarm->Type, insert_alarm->seconds, insert_alarm->message);//the first braket should contain the ID instead of time
                    break;
                }

                last = &next->link;
                next = next->link;
            }

            if (next == NULL) {
                printf("Alarm(%d) not found. Cannot change.\n", alarm_id);
            }

            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0) err_abort(status, "Unlock mutex");

        }
        else if (strncmp(line, "Cancel_Alarm", 12) == 0) {
            int alarm_id;

            if (sscanf(line, "Cancel_Alarm(%d)", &alarm_id) != 1) {
                fprintf(stderr, "Bad Cancel_Alarm command\n");
                continue; // Skip to the next input
            } else {
                printf("Parsed Cancel_Alarm: ID = %d\n", alarm_id);
            }

            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            alarm_t **last = &alarm_list;
            alarm_t *current = alarm_list;
            int found = 0;

            while (current != NULL) {
                if (current->Alarm_ID == alarm_id) {
                    printf("Cancelling Alarm(%d)\n", current->Alarm_ID);

                    *last = current->link;

                    free(current);

                    found = 1;
                    break;
                }

                last = &current->link;
                current = current->link;
            }

            if (!found) {
                printf("Alarm(%d) not found. Cannot cancel.\n", alarm_id);
            }

            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock mutex");
        }
        else if (strncmp(line, "View_Alarms", 11) == 0) {
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");
            //print the  view time 
            time_t view_time = time(NULL);
            printf("View Alarms at %ld:\n", view_time);

            if (alarm_list == NULL) {
                printf("No alarms in the list\n");
            } else {
                //track thread id 
                pthread_t current_thread = 0;
                int display_thread_index = 1;
                int alarm_index = 1;
                printf("Current Alarms in the List:\n");
                alarm_t *current = alarm_list;
                while (current != NULL) {
                    /*
                    printf("Alarm_ID: %d, Type: %s, Seconds: %d, Message: %s, Time: %ld\n",
                        current->Alarm_ID, current->Type, current->seconds, current->message, current->time);
                    */
                   //a3.2.5
                   if (current->display_thread != current_thread) {
                    current_thread = current->display_thread;
                    printf("%d. Display Thread %lu Assigned:\n", display_thread_index, (unsigned long)current_thread);
                    display_thread_index++;
                    alarm_index = 1;
                    }
                    //print assigned alarm
                    printf("%d%c. Alarm(%d): %s %ld %s\n",
                        display_thread_index - 1, 'a' + (alarm_index - 1), 
                        current->Alarm_ID, current->Type, current->time, current->message);
                    //move to next alarm 
                    alarm_index++;
                    current = current->link;  // Move to the next alarm
                }
            }

            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock mutex");
        }
    }
}