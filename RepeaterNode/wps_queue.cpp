/*
 *  WPS_QUEUE.CPP : WPS Common Used Queue Module
 * 
 *  ver        date            author         comment
 *  0.0.1      08/02/26        Gao Hua        First
 */

#include "wps_queue.h"


#ifndef NULL
#define NULL	0
#endif


/* Initialize a wps_queue node ------------------------------------------------- */
void wps_init_que(wps_queue *item, void *body)
{
	item->q_forw = NULL;
	item->q_back = NULL;
	item->q_body = body;
}

/* Insert a item to a wps_queue ------------------------------------------------ */
/*
root	:Pointer to Root pointer
point	:Insert point (item will be link after it)
item	:Item to insert
*/
void wps_insert_que(wps_queue **root, wps_queue *point, wps_queue *item)
{
	if (*root == NULL) {
		item->q_back = NULL;
		item->q_forw = NULL;
		*root =item;
	}
	else {
		if (point == NULL) {
			item->q_back = NULL;
			item->q_forw = *root;
			(*root)->q_back = item;
			*root = item;
		}
		else {
			/* Link forword wps_queue */
			item->q_forw = point->q_forw;
			if (point->q_forw != NULL) point->q_forw->q_back = item;

			/* Link back wps_queue */
			point->q_forw = item;
			item->q_back = point;
		}
	}

	return;
}

/* Append a item to a wps_queue ------------------------------------------------ */
/*
root	:Pointer to Root pointer
item	:Item to append
*/
void wps_append_que(wps_queue **root, wps_queue *item)
{
	wps_queue	*point;

	item->q_forw = NULL;

	if (*root == NULL) {
		item->q_back = NULL;
		*root =item;
	}
	else {
		point = *root;

		while (point->q_forw != NULL) point = point->q_forw;

		/* Link back wps_queue */
		point->q_forw = item;
		item->q_back = point;
	}

	return;
}

/* Delete a item from a wps_queue ---------------------------------------------- */
/*
root	:Pointer to Root pointer
item	:Item to delete
*/
void wps_remove_que(wps_queue **root, wps_queue *item)
{
	/* Delete forword link */
	if (item->q_forw != NULL) item->q_forw->q_back = item->q_back;

	/* Delete back link */
	if (item->q_back == NULL) {
		/*
		 * 1999.07.03
		 * Ensure that the node is really the first node of the link.
		 */
		if (*root == item) *root = item->q_forw;
	}
	else
		item->q_back->q_forw = item->q_forw;

	item->q_back = NULL;
	item->q_forw = NULL;

	return;
}

/* Count number of node in a wps_queue ----------------------------------------- */
int wps_count_que(wps_queue *qp)
{
	int	i = 0;

	while (qp != NULL) {
		qp = qp->q_forw;
		i++;
	}

	return (i);
}

