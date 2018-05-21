#ifndef _VIEWER_IF_H_
#define _VIEWER_IF_H_


//
//  0: OK
// -1: NG
int if_get_viewer_nodeid(char *buff, int size);


void if_show_text(char *utf8Text);


//
//  0: OK
// -1: NG
int if_messagebox(int msg_rid);

//
//  0: OK
// -1: NG
int if_messagetip(int msg_rid);

//
//  0: OK
// -1: NG
int if_progress_show(int msg_rid);

//
//  0: OK
// -1: NG
int if_progress_show_format1(int msg_rid, int arg1);

//
//  0: OK
// -1: NG
int if_progress_show_format2(int msg_rid, int arg1, int arg2);

//
//  0: OK
// -1: NG
int if_progress_cancel();

//
//  0: OK
// -1: NG
int if_on_connected(int type, int fhandle);



#endif /* _VIEWER_IF_H_ */
