/*
 *  WPS_QUEUE.H : WPS Common Used Queue Module
 * 
 *  ver        date            author         comment
 *  0.0.1      08/02/26        Gao Hua        First
 */

#ifndef _WPS_QUEUE_H
#define _WPS_QUEUE_H


typedef struct _tag_wps_queue {
	struct _tag_wps_queue *q_forw;
	struct _tag_wps_queue *q_back;
	void                  *q_body;
} wps_queue;

#define get_qforw(vq) (((vq) == NULL) ? NULL : ((vq)->q_forw))
#define get_qback(vq) (((vq) == NULL) ? NULL : ((vq)->q_back))
#define get_qbody(vq) (((vq) == NULL) ? NULL : ((vq)->q_body))

void wps_init_que(wps_queue *item, void *body);
void wps_insert_que(wps_queue **root, wps_queue *point, wps_queue *item);
void wps_remove_que(wps_queue **root, wps_queue *item);
void wps_append_que(wps_queue **root, wps_queue *item);

int wps_count_que(wps_queue *qp);


#endif /* _WPS_QUEUE_H */
