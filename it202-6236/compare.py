#note: subprocess have been checked, it actually waits the end of the process before giving an output.
import matplotlib.pyplot as plt
import time as tm
import os
import sys
import pylab
import subprocess

# Constant
CRASH_ON_ERROR = False
if len(sys.argv) > 1:
    CRASH_ON_ERROR = int(sys.argv[1]) # You can use : python3 ./compare.py 1
FNULL = open(os.devnull, 'w')
NB_EXECUTIONS = 5
MAX_THREADS   = 26
PTHREAT_SUFFIX = "_pthread"
EXECUTABLES  = [#"21-create-many",                         \
                #"21-create-many"+PTHREAT_SUFFIX,          \
                #"22-create-many-recursive",               \
                #"22-create-many-recursive"+PTHREAT_SUFFIX,\
                #"23-create-many-once",                    \
                #"23-create-many-once"+PTHREAT_SUFFIX,     \
                #"31-switch-many",                         \
                #"31-switch-many"+PTHREAT_SUFFIX,          \
                #"32-switch-many-join",                    \
                #"32-switch-many-join"+PTHREAT_SUFFIX],
                "51-fibonacci",\
                "51-fibonacci"+PTHREAT_SUFFIX]#,     \
               # "61-mutex",                               \
               # "61-mutex"+PTHREAT_SUFFIX]
ARGUMENTS = [#"",\
             #"",\
             #"",\
             #"20",\
             "20"]#,\
             #""]
BLD_DIR = "build"

# Error if ARGUMENTS' lentgh doesn't match EXECUTABLES' length
if (len(ARGUMENTS) != len(EXECUTABLES)/2):
    print("ERROR: len(ARGUMENTS) != len(EXECUTABLES)/2)")
    exit()

# Variables
compile_commands = ["make", "make pthreads"]
x     = range(1, MAX_THREADS + 1)
times = [[0 for j in range(len(x))] for i in range(len(EXECUTABLES))]
width = 1.


#========================================================
# FUNCTIONS
#========================================================

def get_execution_time(command_line, arguments, nb_thread, nb_execution):
    error_occured = False

    #Getting Execution time
    t0 = tm.clock()
    if CRASH_ON_ERROR:
        for i in range(nb_execution):
            error_occured = abs(subprocess.check_call(command_line+' '+arguments+' '+str(nb_thread), shell=True, stdout=FNULL, stderr=FNULL))
    else:
        for i in range(nb_execution):
            error_occured = abs(subprocess.call(command_line+' '+arguments+' '+str(nb_thread), shell=True, stdout=FNULL, stderr=FNULL))
    t1 = tm.clock()

    if error_occured:
        return 0
    return (t1 - t0)/nb_execution


#========================================================
# MAIN
#========================================================

# Compiling executables
for i in range(len(compile_commands)):
    os.system(compile_commands[i])

# Comparing executables
n_exec = 0
n_args = 0
while n_exec < len(EXECUTABLES):
    #Retrieving executables to compare
    exec_our = EXECUTABLES[n_exec]
    exec_pthread = EXECUTABLES[n_exec + 1]
    args   = ARGUMENTS[n_args]

    # Timing executions
    print(str(n_args)+" - Executing "+exec_our+" "+args+" and "+exec_pthread+" "+args+" ...")
    for n_thread in range(1, MAX_THREADS + 1):
        args = str(n_thread)
        times[n_exec][n_thread - 1]     = get_execution_time(BLD_DIR+'/'+exec_our, args, n_thread, NB_EXECUTIONS)
        times[n_exec + 1][n_thread - 1] = get_execution_time(BLD_DIR+'/'+exec_pthread, args, n_thread, NB_EXECUTIONS)
        #Feedback showing script's advancement
        sys.stdout.write("\r")
        sys.stdout.write("\tnb threads : "+str(n_thread)+" ("+str(100*n_thread/MAX_THREADS)+" %)")
        sys.stdout.flush()
    print("\n\tdone")

    #Next executable
    n_exec += 2
    n_args += 1


#========================================================
# CREATING FIGURES
#========================================================

print("Creating figure...")
n_exec = 0
while n_exec < len(EXECUTABLES):
    #Creating new figure
    plt.figure()

    #Filling figure
    plt.plot(x, times[n_exec], color = "SkyBlue", label='not pthread')
    plt.plot(x, times[n_exec + 1], color = "IndianRed", label='pthread')

    #Fancyness, legends and titles
    #plt.ylim(0,max(times))
    #plt.xlim(0, len(x) + 1)
    plt.title(EXECUTABLES[n_exec])
    plt.ylabel("Time (s)")
    plt.xlabel("Test Executable")
    plt.grid()

    n_exec += 2

plt.show()
