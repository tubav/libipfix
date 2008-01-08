/* 
 * IPFIX structs, types and definitions
 *
 * This is a generated file. Do not edit! 
 *
 */

/* ipfix information element list
 */
ipfix_field_type_t ipfix_ft_fokus[] = {
   { 12325, IPFIX_FT_REVOCTETDELTACOUNT, 8, IPFIX_CODING_UINT, 
     "revOctetDeltaCount", "FOKUS reverse delta octet count" }, 
   { 12325, IPFIX_FT_REVPACKETDELTACOUNT, 8, IPFIX_CODING_UINT, 
     "revPacketDeltaCount", "FOKUS reverse delta packet count" }, 
   { 12325, IPFIX_FT_RTTMEAN_USEC, 8, IPFIX_CODING_UINT, 
     "rttmean_usec", "FOKUS mean rtt in us" }, 
   { 12325, IPFIX_FT_RTTMIN_USEC, 8, IPFIX_CODING_UINT, 
     "rttmin_usec", "FOKUS minimum rtt in us" }, 
   { 12325, IPFIX_FT_RTTMAX_USEC, 8, IPFIX_CODING_UINT, 
     "rttmax_usec", "FOKUS maximum rtt in us" }, 
   { 12325, IPFIX_FT_IDENT, 65535, IPFIX_CODING_STRING, 
     "ident", "FOKUS ident" }, 
   { 12325, IPFIX_FT_LOSTPACKETS, 4, IPFIX_CODING_INT, 
     "lostPackets", "FOKUS lost packets" }, 
   { 12325, IPFIX_FT_OWDVAR_USEC, 4, IPFIX_CODING_INT, 
     "owdvar_usec", "FOKUS delay variation in us" }, 
   { 12325, IPFIX_FT_OWDVARMEAN_USEC, 4, IPFIX_CODING_INT, 
     "owdvarmean_usec", "FOKUS mean dvar in us" }, 
   { 12325, IPFIX_FT_OWDVARMIN_USEC, 4, IPFIX_CODING_INT, 
     "owdvarmin_usec", "FOKUS minimum dvar in us" }, 
   { 12325, IPFIX_FT_OWDVARMAX_USEC, 4, IPFIX_CODING_INT, 
     "owdvarmax_usec", "FOKUS maximum dvar in us" }, 
   { 12325, IPFIX_FT_OWDSD_USEC, 8, IPFIX_CODING_UINT, 
     "owdsd_usec", "FOKUS owd standard deviation" }, 
   { 12325, IPFIX_FT_OWD_USEC, 8, IPFIX_CODING_UINT, 
     "owd_usec", "FOKUS one-way-delay in us" }, 
   { 12325, IPFIX_FT_OWDMEAN_USEC, 8, IPFIX_CODING_UINT, 
     "owdmean_usec", "FOKUS mean owd in us" }, 
   { 12325, IPFIX_FT_OWDMIN_USEC, 8, IPFIX_CODING_UINT, 
     "owdmin_usec", "FOKUS minimum owd in us" }, 
   { 12325, IPFIX_FT_OWDMAX_USEC, 8, IPFIX_CODING_UINT, 
     "owdmax_usec", "FOKUS maximum owd in us" }, 
   { 12325, IPFIX_FT_TASKID, 4, IPFIX_CODING_UINT, 
     "taskId", "FOKUS task id" }, 
   { 12325, IPFIX_FT_TSTAMP_SEC, 4, IPFIX_CODING_INT, 
     "tstamp_sec", "FOKUS tstamp" }, 
   { 12325, IPFIX_FT_TSTAMP_NSEC, 4, IPFIX_CODING_INT, 
     "tstamp_nsec", "FOKUS tstamp nanosecond fraction" }, 
   { 12325, IPFIX_FT_PKTLENGTH, 4, IPFIX_CODING_INT, 
     "pktLength", "FOKUS packet length" }, 
   { 12325, IPFIX_FT_PKTID, 4, IPFIX_CODING_UINT, 
     "pktId", "FOKUS packet id" }, 
   { 12325, IPFIX_FT_STARTTIME, 4, IPFIX_CODING_INT, 
     "startTime", "FOKUS interval start" }, 
   { 12325, IPFIX_FT_ENDTIME, 4, IPFIX_CODING_INT, 
     "endTime", "FOKUS interval end" }, 
   { 12325, IPFIX_FT_FLOWCREATIONTIMEUSEC, 4, IPFIX_CODING_INT, 
     "flowCreationTimeUsec", "FOKUS flow start usec fraction" }, 
   { 12325, IPFIX_FT_FLOWENDTIMEUSEC, 4, IPFIX_CODING_INT, 
     "flowEndTimeUsec", "FOKUS flow end usec fraction" }, 
   { 0, 0, -1, 0, NULL, NULL, }
};
