iscsiadm  --mode node --targetname iqn.2006-01.com.openfiler:tsn.20 --portal 192.168.0.20 --login
iscsiadm  --mode node --targetname iqn.2006-01.com.openfiler:tsn.21 --portal 192.168.0.21 --login
iscsiadm  --mode node --targetname iqn.2006-01.com.openfiler:tsn.22 --portal 192.168.0.22 --login
iscsiadm  --mode node --targetname iqn.2006-01.com.openfiler:tsn.23 --portal 192.168.0.23 --login
iscsiadm  --mode node --targetname iqn.2006-01.com.openfiler:tsn.24 --portal 192.168.0.24 --login
iscsiadm  --mode node --targetname iqn.2006-01.com.openfiler:tsn.25 --portal 192.168.0.25 --login

mount /dev/sdb /disks/chocolate
mount /dev/sdc /disks/strawberry
mount /dev/sdd /disks/vanilla
mount /dev/sde /disks/yam
mount /dev/sdf /disks/durian
mount /dev/sdg /disks/mango
