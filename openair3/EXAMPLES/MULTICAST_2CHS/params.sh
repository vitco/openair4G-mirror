#!/bin/bash

MR1_IN_ADDR=10.0.1.3
MR1_IN6_ADDR=2001:10:0:1:7856:3412:0:2
MR1_EG_ADDR=192.168.9.1
MR1_EG6_ADDR=2001:660:5502::100
# Flow CH1->MR1
MR1_LABEL_IN=1001
# Flow MR1->CH1
MR1_LABEL_OUT=1000
ETH_MR1=eth2

MR2_IN_ADDR=10.0.1.4
MR2_IN6_ADDR1=2001:10:0:1:7856:3412:0:3
MR2_IN6_ADDR2=2001:10:0:2:7856:3412:0:3
MR2_EG6_ADDR=2001:660:5502::300
# Flow CH1->MR2
MR2_CH1_LABEL_IN=2000
# Flow MR2->CH1
MR2_CH1_LABEL_OUT=2001
# Flow CH2->MR2
MR2_CH2_LABEL_IN=3000
# Flow MR2->CH2
MR2_CH2_LABEL_OUT=3001

MR3_IN_ADDR=10.0.1.5
MR3_IN6_ADDR=2001:10:0:2:7856:3412:0:2
MR3_EG_ADDR=192.168.10.1
MR3_EG6_ADDR=2001:660:5502::200
# Flow CH2->MR3
MR3_LABEL_IN=4000
# Flow MR3->CH2
MR3_LABEL_OUT=4001
ETH_MR3=eth3

CH1_IN_ADDR=10.0.1.1
CH2_IN_ADDR=10.0.1.2

CH1_IN6_ADDR=2001:10:0:1:7856:3412:0:1
CH1_EG6_ADDR=2001:660:5502::400
CH2_IN6_ADDR=2001:10:0:2:7856:3412:0:1

## Communication between the CHs
CH1_MR2_CH2_LABEL_IN=8001
CH1_MR2_CH2_LABEL_OUT=8000
CH2_MR2_CH1_LABEL_IN=9001
CH2_MR2_CH1_LABEL_OUT=9000

#MN1_IN6_ADDR=2001:660:5502::20
#MN2_IN6_ADDR=2001:660:5502::25
MN1_IN6_ADDR=2001:660:5502::110
MN1_IN_ADDR=192.168.9.2

MN2_IN6_ADDR=2001:660:5502::210
MN3_IN6_ADDR=2001:660:5502::220
MN2_IN_ADDR=192.168.10.2

# Put the right OPENAIR3 path here
OPENAIR3_HOME=/homes/foubert/Openair/openair3
OPENAIR3_PMIP6D_PATH=$OPENAIR3_HOME/PMIP6D
#OPENAIR3_SCRIPTS_PATH=$OPENAIR3_HOME/scripts
#REFLECTOR_DIR=$OPENAIR3_HOME/EXAMPLES/MULTICAST_2CHS/REFLECTOR

# Put the right OPENAIR2 path here
#OPENAIR2_DIR=~/openair2_r234/openair2/
#OPENAIR2_DIR=~/CHORIST_2CHs_EMULATION_MULTICAST_20090119/openair2

# Multicast Reflector
#REFLECTOR_DIR=~/CHORIST_2CHs_EMULATION_MULTICAST_20090119/openair3/EXAMPLES/MULTICAST_2CHS/REFLECTOR
