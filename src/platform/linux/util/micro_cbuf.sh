#!/bin/sh

# ping pong

./cos_loader \
"c0.o, ;llboot.o, ;*fprr.o, ;mm.o, ;print.o, ;boot.o, ;\
\
!mpool.o,a3;!trans.o,a6;!sm.o,a4;!l.o,a1;!te.o,a3;!e.o,a4;!stat.o,a25;!buf.o,a5;(!rend.o=micro_ppong.o), ;(!pi.o=micro_pingp.o), 'large_run';(!po.o=micro_pingp.o), 'large_run_separate';!va.o,a2;!vm.o,a2:\
\
c0.o-llboot.o;\
fprr.o-print.o|[parent_]mm.o|[faulthndlr_]llboot.o;\
mm.o-[parent_]llboot.o|print.o;\
boot.o-print.o|fprr.o|mm.o|llboot.o;\
l.o-fprr.o|mm.o|print.o;\
te.o-sm.o|print.o|fprr.o|mm.o|va.o;\
e.o-sm.o|fprr.o|print.o|mm.o|l.o|va.o;\
stat.o-sm.o|te.o|fprr.o|l.o|print.o|e.o;\
sm.o-print.o|fprr.o|mm.o|boot.o|va.o|l.o|mpool.o;\
buf.o-boot.o|sm.o|fprr.o|print.o|l.o|mm.o|va.o|mpool.o;\
mpool.o-print.o|fprr.o|mm.o|boot.o|va.o|l.o;\
va.o-fprr.o|print.o|mm.o|l.o|boot.o;\
trans.o-sm.o|fprr.o|l.o|buf.o|mm.o|va.o|e.o|print.o;\
vm.o-sm.o|fprr.o|print.o|mm.o|l.o;\
rend.o-sm.o|print.o|mm.o|va.o|buf.o|l.o|fprr.o;\
pi.o-sm.o|fprr.o|va.o|print.o|mm.o|l.o|buf.o|te.o|vm.o|rend.o;\
po.o-sm.o|fprr.o|va.o|print.o|mm.o|l.o|buf.o|te.o|vm.o|rend.o\
" ./gen_client_stub
