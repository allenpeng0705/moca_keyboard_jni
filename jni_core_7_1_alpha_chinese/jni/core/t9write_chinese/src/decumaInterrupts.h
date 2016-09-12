#ifdef PROFILE_ABORT_RECOGNITION
/* With this compiler option set, we will send the code position as a string as the user data to the callback function
 * which can then use this string to generate statistics about between which lines that we have the longest elapse
 * between abort recognition callbacks
 */
#define MY_STRINGIFY(x) #x
#define MY_TOSTRING(x) MY_STRINGIFY(x)
#define CALL_ABORT_RECOGNITION_CALLBACK(pIF) ((pIF)->pShouldAbortRecognize((void *) __FILE__ MY_TOSTRING(__LINE__)))

#else
/* Normal case: Check with the registered callback (if exists) if we should continue
 * Returns 1 if we should abort
 */
#define CALL_ABORT_RECOGNITION_CALLBACK(pIF) ((pIF)->pShouldAbortRecognize((pIF)->pUserData))
#endif

#define TEST_ABORT_RECOGNITION(pIF) ((pIF) != NULL && CALL_ABORT_RECOGNITION_CALLBACK(pIF)!=0)

/* Extra macro to define that the check should only be done with certain intervals
 * This is useful to be able to tune the frequence of callbacks inside certain loops
 * This macro needs a counter to be provided
 * Returns 1 if we should abort
 */
#define TEST_ABORT_RECOGNITION_EVERY(INTERVAL,COUNTER,pIF) ((pIF)!= NULL && (COUNTER)%(INTERVAL)==0 && CALL_ABORT_RECOGNITION_CALLBACK(pIF)!=0)
