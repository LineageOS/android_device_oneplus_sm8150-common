#!/system/bin/sh

PATH=/data/coredump
MAX_NUM_APP_CORE_FILES=20 # Set limit of all app coredump files
LIMIT_TIME=600 # Set coredump interval limit to 10 minutes to avoid dump to many times

pid=$1          # process id
uid=$2          # user id
ts=$3           # time stamp

# Get process name from /proc/$pid/cmdline
pname=""
fcmdline=/proc/$pid/cmdline
isExist=(`/system/bin/ls /proc/$pid`)
if [ -n $isExist ]; then
  read pname < $fcmdline
else
  echo "File /proc/$pid/cmdline doesn't exist" >> $LOG
fi

# To seperate system_server and other app process
if [ "$pname" != "system_server" ]; then
    fname=$ts.$pid.$uid-$pname.app.core.gz
else
    fname=$ts.$pid.$uid-$pname.core.gz  
fi

OUTPUT=$PATH/$fname # absolute file name
LOG=$PATH/$ts.$pid.$uid.tmp.log # Log path
/system/bin/rm $PATH/*.tmp.log

# create the empty log
/system/bin/touch $LOG

echo "Generating coredump file of $pname ..." >> $LOG

# Check last coredump time, if is app. limit the coredump interval 
if [ "$pname" != "system_server" ]; then
    loglist=(`/system/bin/ls $PATH/*core.gz.log`)
    if [ -n $loglist -a ${#loglist[@]} -gt 0 ]; then
        lastIndex=$((${#loglist[@]} - 1))
        lastlog=${loglist[$lastIndex]}
        echo "Check lastlog $lastlog" >> $LOG
        logfile=${lastlog#$PATH/*}
        lastTime=${logfile%%.*}

        if [ $(($ts - $lastTime)) -lt $LIMIT_TIME ]; then 
            echo "Skip! It had done core dump within $LIMIT_TIME seconds ago." >> $LOG
            echo "$ts $pid $uid $pname (skip dump)" >> $PATH/core_history
            exit 1
        fi
    fi
fi

# Only keep the last coredump file per app except system_server

if [ "$pname" != "system_server" ]; then
    loglist=(`/system/bin/ls $PATH/*.app.core.gz`) # filter app core logs
    if [ -n $loglist ]; then
        for logfile in $loglist
        do
            read _pname < $logfile.log
            if [ "$_pname" = "$pname" ]; then
                echo "delete core files of the same APP : $pname" >> $LOG
                echo "delete $logfile.log" >> $LOG
                echo "delete $logfile" >> $LOG
                /system/bin/rm $logfile.log
                /system/bin/rm $logfile
                break
            fi
        done
    fi
fi

# Rename the log file
/system/bin/mv $LOG $PATH/$fname.log
LOG=$PATH/$fname.log


# Check whether total app coredump file count exceeds the max file count
maxFiles=$MAX_NUM_APP_CORE_FILES

filelist=(`/system/bin/ls $PATH/*app.core.gz`)
if [ -n $filelist ]; then
    num_to_delete=$((${#filelist[@]} - $maxFiles + 1))
    echo "Current total app coredump count : ${#filelist[@]}, Max count : $maxFiles" >> $LOG

    if [ $num_to_delete -gt 0 ]; then
        index=0
        while [ $index -lt $num_to_delete ];
        do
            delete_fname=${filelist[$index]}
            if [ -n "$delete_fname" ]; then
                echo "FileControl - $index) delete '$delete_fname'" >> $LOG
                /system/bin/rm $delete_fname
                /system/bin/rm $delete_fname.log
            fi
            index=$(($index + 1))
        done
    fi
fi
#}

# Delete the redundant core.dump.maps.PID.TID file
cd $PATH
filelist=(`/system/bin/ls *.core.gz`)
if [ -n $filelist ]; then
    for flog in "${filelist[@]}"
    do
        #echo "$flog"
        file_tmp=${flog#*.}
        file_pid=${file_tmp%%.*}
        echo "pid $file_pid"
        target=(`/system/bin/ls core.dump.maps.$file_pid.*`)
        if [ -n $target ];
        then
           /system/bin/mv $target "s.$target" # prefix s. marked
           echo "rename s.$target"
        fi
    done
fi
/system/bin/rm core.dump.maps.*
filelist=(`/system/bin/ls s.core.dump.maps.*`)
if [ -n $filelist ]; then
    for flog in "${filelist[@]}"
    do
        /system/bin/mv $flog ${flog#*.}
        echo "rename ${flog#*.}"
    done
fi
cd /
#}

echo "Generating core files..." >> $LOG

echo "process name = $pname" >>$LOG
echo "pid = $pid" >> $LOG
echo "uid = $uid" >> $LOG

/system/bin/gzip -1 > $OUTPUT
echo "Zip coredump file done..." >> $LOG
echo "$ts $pid $uid $pname" >> $PATH/coredump_history
