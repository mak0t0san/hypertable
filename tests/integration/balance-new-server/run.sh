#!/usr/bin/env bash

HT_HOME=${INSTALL_DIR:-"$HOME/hypertable/current"}
HYPERTABLE_HOME=${HT_HOME}
HT_SHELL=$HT_HOME/bin/hypertable
SCRIPT_DIR=`dirname $0`
#DATA_SEED=42 # for repeating certain runs
WRITE_SIZE=${WRITE_SIZE:-"40000000"}
RS1_PIDFILE=$HT_HOME/run/Hypertable.RangeServer.rs1.pid
RS2_PIDFILE=$HT_HOME/run/Hypertable.RangeServer.rs2.pid
RUN_DIR=`pwd`

. $HT_HOME/bin/ht-env.sh

stop_range_servers() {
  echo "shutdown; quit;" | $HT_HOME/bin/ht rsclient localhost:38061
  echo "shutdown; quit;" | $HT_HOME/bin/ht rsclient localhost:38060

  sleep 1

  kill -9 `cat $HT_HOME/run/Hypertable.RangeServer.rs?.pid`
}

# stop and start servers
rm metadata.*
$HT_HOME/bin/start-test-servers.sh --no-rangeserver --no-thriftbroker \
    --clear --config=${SCRIPT_DIR}/test.cfg
$HT_HOME/bin/ht Hypertable.RangeServer --verbose --pidfile=$RS1_PIDFILE \
   --Hypertable.RangeServer.ProxyName=rs1 \
   --Hypertable.RangeServer.Port=38060 --config=${SCRIPT_DIR}/test.cfg 2>1 > rangeserver.rs1.output&

#create table
$HT_HOME/bin/ht shell --no-prompt < $SCRIPT_DIR/create-table.hql

#write data 
$HT_HOME/bin/ht ht_load_generator update \
    --Hypertable.Mutator.FlushDelay=50 \
    --rowkey.component.0.order=random \
    --rowkey.component.0.type=integer \
    --rowkey.component.0.format="%010lld" \
    --rowkey.component.0.min=0 \
    --rowkey.component.0.max=1000000 \
    --row-seed=1 \
    --Field.value.size=20000 \
    --max-bytes=$WRITE_SIZE

#dump METADATA location
sleep 30
${HT_HOME}/bin/ht shell --no-prompt --exec "use sys; select Location from METADATA MAX_VERSIONS=1 into file '${RUN_DIR}/metadata.pre';"

#start new rangeserver
$HT_HOME/bin/ht Hypertable.RangeServer --verbose --pidfile=$RS2_PIDFILE \
   --Hypertable.RangeServer.ProxyName=rs2 \
   --Hypertable.RangeServer.Port=38061 --config=${SCRIPT_DIR}/test.cfg 2>1 > rangeserver.rs2.output&

#wait for balance to complete 
sleep 30 

#dump METADATA location
${HT_HOME}/bin/ht shell --no-prompt --exec "use sys; select Location from METADATA MAX_VERSIONS=1 into file '${RUN_DIR}/metadata.post';"

#shut everything down
stop_range_servers
$HT_HOME/bin/clean-database.sh

#make sure ranges are evenly divided
TOTAL_RANGES=`grep Location metadata.post|wc -l`
MOVED_RANGES=`grep "rs2" metadata.post|wc -l`
echo "Total ranges=${TOTAL_RANGES} moved ranges=${MOVED_RANGES}"
#somewhere between 40-50% of the ranges should be on the new server
if (( $MOVED_RANGES > 2*$TOTAL_RANGES/5  && $MOVED_RANGES < $TOTAL_RANGES/2))
then
  echo "Test passed"
else 
  echo "Test failed"
  exit 1
fi

exit 0
