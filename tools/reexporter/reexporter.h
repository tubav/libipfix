#ifndef REEXPORTER_H
#define	REEXPORTER_H

void exit_func(int);
int export_newsource_cb(ipfixs_node_t*, void*);
int export_newmsg_cb(ipfixs_node_t*, ipfix_hdr_t*, void*);
int export_trecord_cb(ipfixs_node_t*, ipfixt_node_t*, void*);
int export_drecord_cb(ipfixs_node_t*, ipfixt_node_t*, ipfix_datarecord_t*, void*);
void export_cleanup_cb(void*);
void *startReexporter();

#endif
