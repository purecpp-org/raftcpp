
nohup ./example_counter_server_main --conf="127.0.0.1:10001,127.0.0.1:10002,127.0.0.1:10003" >> ./nohup_10001_output.log 2>&1 &
nohup ./example_counter_server_main --conf="127.0.0.1:10002,127.0.0.1:10001,127.0.0.1:10003" >> ./nohup_10002_output.log 2>&1 &
nohup ./example_counter_server_main --conf="127.0.0.1:10003,127.0.0.1:10002,127.0.0.1:10001" >> ./nohup_10003_output.log 2>&1 &

